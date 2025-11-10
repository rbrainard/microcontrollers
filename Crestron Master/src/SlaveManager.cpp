#include "SlaveManager.h"
#include <esp_log.h>
#include <algorithm>

static const char* TAG = "SlaveManager";

SlaveManager::SlaveManager(RS485Communication& rs485)
    : m_rs485(rs485)
    , m_taskHandle(nullptr)
    , m_pingTimer(nullptr)
    , m_commandQueue(nullptr)
    , m_slavesMutex(nullptr)
    , m_initialized(false)
    , m_pingEnabled(false)
    , m_currentSlaveIndex(0)
    , m_pingSequenceNumber(0)
    , m_totalPings(0)
    , m_successfulPings(0)
    , m_configurationCount(0)
{
}

SlaveManager::~SlaveManager() {
    deinitialize();
}

bool SlaveManager::initialize() {
    if (m_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    // Create FreeRTOS objects
    m_commandQueue = xQueueCreate(16, sizeof(Command));
    m_slavesMutex = xSemaphoreCreateMutex();
    
    if (!m_commandQueue || !m_slavesMutex) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS objects");
        deinitialize();
        return false;
    }

    // Initialize configuration templates
    initializeConfigTemplates();

    // Create ping timer (but don't start it yet)
    m_pingTimer = xTimerCreate("PingTimer", 
                               pdMS_TO_TICKS(CrestronTiming::PING_INTERVAL_MS),
                               pdTRUE, // Auto-reload
                               this,   // Timer ID
                               pingTimerCallback);

    if (!m_pingTimer) {
        ESP_LOGE(TAG, "Failed to create ping timer");
        deinitialize();
        return false;
    }

    // Create main task
    if (xTaskCreate(taskFunction, "SlaveManager", StackSizes::SLAVE_MANAGER, 
                    this, TaskPriorities::SLAVE_MANAGER, &m_taskHandle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create slave manager task");
        deinitialize();
        return false;
    }

    m_initialized = true;
    ESP_LOGI(TAG, "Slave manager initialized successfully");
    return true;
}

void SlaveManager::deinitialize() {
    if (!m_initialized) return;

    enablePinging(false);

    if (m_taskHandle) {
        vTaskDelete(m_taskHandle);
        m_taskHandle = nullptr;
    }

    if (m_pingTimer) {
        xTimerDelete(m_pingTimer, portMAX_DELAY);
        m_pingTimer = nullptr;
    }

    if (m_commandQueue) {
        vQueueDelete(m_commandQueue);
        m_commandQueue = nullptr;
    }

    if (m_slavesMutex) {
        vSemaphoreDelete(m_slavesMutex);
        m_slavesMutex = nullptr;
    }

    m_slaves.clear();
    m_initialized = false;
    
    ESP_LOGI(TAG, "Slave manager deinitialized");
}

bool SlaveManager::addSlave(uint8_t address, SlaveType type) {
    if (!m_initialized) return false;

    if (xSemaphoreTake(m_slavesMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        SlaveInfo slave = {
            .address = address,
            .type = type,
            .state = SlaveState::OFFLINE,
            .configStep = ConfigStep::NONE,
            .lastPingTime = 0,
            .configStepIndex = 0,
            .dimRequest1 = false,
            .dimRequest2 = false,
            .errorCount = 0
        };

        m_slaves[address] = slave;
        xSemaphoreGive(m_slavesMutex);
        
        ESP_LOGI(TAG, "Added slave 0x%02X (type %d)", address, static_cast<int>(type));
        return true;
    }
    
    return false;
}

bool SlaveManager::removeSlave(uint8_t address) {
    if (!m_initialized) return false;

    if (xSemaphoreTake(m_slavesMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        auto it = m_slaves.find(address);
        if (it != m_slaves.end()) {
            m_slaves.erase(it);
            xSemaphoreGive(m_slavesMutex);
            ESP_LOGI(TAG, "Removed slave 0x%02X", address);
            return true;
        }
        xSemaphoreGive(m_slavesMutex);
    }
    
    return false;
}

void SlaveManager::enablePinging(bool enable) {
    if (!m_initialized) return;

    m_pingEnabled = enable;
    
    if (enable) {
        xTimerStart(m_pingTimer, 0);
        ESP_LOGI(TAG, "Pinging enabled");
    } else {
        xTimerStop(m_pingTimer, 0);
        ESP_LOGI(TAG, "Pinging disabled");
    }
}

bool SlaveManager::sendDimCommand(uint8_t address, uint8_t channel, uint8_t level, uint16_t rampTime) {
    Command cmd;
    cmd.type = Command::DIM_COMMAND;
    cmd.address = address;
    cmd.channel = channel;
    cmd.level = level;
    cmd.rampTime = rampTime;
    
    return xQueueSend(m_commandQueue, &cmd, pdMS_TO_TICKS(50)) == pdTRUE;
}

bool SlaveManager::sendDimUCommand(uint8_t address, uint8_t channel, uint8_t level, uint16_t rampTime) {
    Command cmd;
    cmd.type = Command::DIMU_COMMAND;
    cmd.address = address;
    cmd.channel = channel;
    cmd.level = level;
    cmd.rampTime = rampTime;
    
    return xQueueSend(m_commandQueue, &cmd, pdMS_TO_TICKS(50)) == pdTRUE;
}

SlaveManager::SlaveState SlaveManager::getSlaveState(uint8_t address) const {
    if (xSemaphoreTake(m_slavesMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        auto it = m_slaves.find(address);
        SlaveState state = (it != m_slaves.end()) ? it->second.state : SlaveState::OFFLINE;
        xSemaphoreGive(m_slavesMutex);
        return state;
    }
    
    return SlaveState::OFFLINE;
}

std::vector<uint8_t> SlaveManager::getOnlineSlaves() const {
    std::vector<uint8_t> online;
    
    if (xSemaphoreTake(m_slavesMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (const auto& pair : m_slaves) {
            if (pair.second.state == SlaveState::ONLINE || 
                pair.second.state == SlaveState::CONFIGURED) {
                online.push_back(pair.first);
            }
        }
        xSemaphoreGive(m_slavesMutex);
    }
    
    return online;
}

// Static task functions
void SlaveManager::taskFunction(void* parameter) {
    SlaveManager* instance = static_cast<SlaveManager*>(parameter);
    instance->handleTask();
}

void SlaveManager::pingTimerCallback(TimerHandle_t timer) {
    SlaveManager* instance = static_cast<SlaveManager*>(pvTimerGetTimerID(timer));
    if (instance && instance->m_pingEnabled) {
        // Queue a ping command
        Command cmd;
        cmd.type = Command::PING;
        xQueueSend(instance->m_commandQueue, &cmd, 0);
    }
}

void SlaveManager::handleTask() {
    Command command;
    RS485Communication::Message rxMessage;
    
    while (true) {
        // Process incoming messages (non-blocking)
        while (m_rs485.receiveMessage(rxMessage, 0)) {
            handleIncomingMessage(rxMessage);
        }
        
        // Process queued commands
        if (xQueueReceive(m_commandQueue, &command, pdMS_TO_TICKS(10)) == pdTRUE) {
            switch (command.type) {
                case Command::PING:
                    processPingCycle();
                    break;
                    
                case Command::DIM_COMMAND: {
                    // Build DIM command message
                    uint8_t dimData[10] = {
                        command.address, 0x08, 0x1D, 0x00,
                        static_cast<uint8_t>(command.rampTime >> 8),
                        static_cast<uint8_t>(command.rampTime & 0xFF),
                        0x00, command.level, command.channel, command.level
                    };
                    m_rs485.sendMessage(dimData, sizeof(dimData));
                    break;
                }
                
                case Command::DIMU_COMMAND: {
                    // Build DIMU command message
                    uint8_t dimUData[13] = {
                        command.address, 0x0B, 0x20, 0x01, 0x08, 0x1D, 0x00,
                        static_cast<uint8_t>(command.rampTime >> 8),
                        static_cast<uint8_t>(command.rampTime & 0xFF),
                        0x00, command.level, command.channel, command.level
                    };
                    m_rs485.sendMessage(dimUData, sizeof(dimUData));
                    break;
                }
                
                case Command::CONFIG_STEP:
                    processConfigurationStep(command.address);
                    break;
            }
        }
    }
}

void SlaveManager::processPingCycle() {
    if (m_slaves.empty()) return;

    if (xSemaphoreTake(m_slavesMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        // Get current slave to ping
        auto it = m_slaves.begin();
        std::advance(it, m_currentSlaveIndex % m_slaves.size());
        
        uint8_t address = it->first;
        SlaveInfo& slave = it->second;
        
        // Check for ping timeout on previously sent pings
        if (slave.state == SlaveState::PING_SENT && isPingTimeout(slave)) {
            handlePingTimeout(address);
        }
        
        // Send ping if slave is offline or online (not during configuration)
        if (slave.state == SlaveState::OFFLINE || 
            slave.state == SlaveState::ONLINE ||
            slave.state == SlaveState::CONFIGURED) {
            
            if (sendPingToSlave(address)) {
                slave.state = SlaveState::PING_SENT;
                slave.lastPingTime = xTaskGetTickCount();
                m_totalPings++;
            }
        }
        
        // Move to next slave
        m_currentSlaveIndex = (m_currentSlaveIndex + 1) % m_slaves.size();
        
        xSemaphoreGive(m_slavesMutex);
    }
}

void SlaveManager::initializeConfigTemplates() {
    // This would contain the configuration sequences from your original code
    // Simplified here for brevity - you'd expand this with all the config data
    
    // DIM8 configuration template
    ConfigData dim8Config;
    dim8Config.type = SlaveType::DIM8;
    // Add all the dimSetup sequences here
    
    // DIMU8 configuration template  
    ConfigData dimU8Config;
    dimU8Config.type = SlaveType::DIMU8;
    // Add all the dimUSetup sequences here
    
    m_configTemplates[SlaveType::DIM8] = dim8Config;
    m_configTemplates[SlaveType::DIMU8] = dimU8Config;
}

bool SlaveManager::sendPingToSlave(uint8_t address) {
    return m_rs485.sendPing(address);
}

bool SlaveManager::sendTimeSync(uint8_t address) {
    uint8_t timeSyncData[10] = {
        address, 0x08, 0x08, 0x0E, 0x15, 0x45, 0x29, 0x05, 0x20, 0x20
    };
    return m_rs485.sendMessage(timeSyncData, sizeof(timeSyncData));
}

void SlaveManager::handleIncomingMessage(const RS485Communication::Message& message) {
    // Parse incoming messages and update slave states
    // This would implement the protocol parsing from your checkResp() function
    
    if (message.length >= 2) {
        uint8_t prefix = message.data[0];
        uint8_t command = message.data[1];
        
        if (prefix == CrestronProtocol::TO_MASTER_PREFIX && command == CrestronProtocol::PING_COMMAND) {
            // Ping response received
            m_successfulPings++;
            
            if (xSemaphoreTake(m_slavesMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                // Find the slave that was pinged and update its state
                for (auto& pair : m_slaves) {
                    if (pair.second.state == SlaveState::PING_SENT) {
                        pair.second.state = SlaveState::ONLINE;
                        break;
                    }
                }
                xSemaphoreGive(m_slavesMutex);
            }
        }
        // Add more message parsing logic here
    }
}

bool SlaveManager::isPingTimeout(const SlaveInfo& slave) const {
    uint32_t elapsed = xTaskGetTickCount() - slave.lastPingTime;
    return elapsed > pdMS_TO_TICKS(CrestronTiming::PING_TIMEOUT_MS);
}

void SlaveManager::handlePingTimeout(uint8_t address) {
    if (xSemaphoreTake(m_slavesMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        auto it = m_slaves.find(address);
        if (it != m_slaves.end()) {
            it->second.state = SlaveState::OFFLINE;
            it->second.errorCount++;
            
            // Send break signal on timeout
            m_rs485.sendBreak();
        }
        xSemaphoreGive(m_slavesMutex);
    }
}

void SlaveManager::processConfigurationStep(uint8_t address) {
    // Implement configuration step processing
    // This would break down the large configuration sequences into smaller steps
}