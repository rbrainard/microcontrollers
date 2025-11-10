#include "ProtocolHandler.h"
#include <esp_log.h>

static const char* TAG = "ProtocolHandler";

ProtocolHandler::ProtocolHandler(SlaveRS485& rs485)
    : m_rs485(rs485)
    , m_taskHandle(nullptr)
    , m_commandQueue(nullptr)
    , m_stateMutex(nullptr)
    , m_initialized(false)
    , m_deviceState(DeviceState::OFFLINE)
    , m_switchState(false)
    , m_deviceAddress(SlaveConfig::DEVICE_ADDRESS)
    , m_commandCount(0)
    , m_pingCount(0)
    , m_errorCount(0)
{
}

ProtocolHandler::~ProtocolHandler() {
    deinitialize();
}

bool ProtocolHandler::initialize() {
    if (m_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    // Create FreeRTOS objects
    m_commandQueue = xQueueCreate(8, sizeof(Command));
    m_stateMutex = xSemaphoreCreateMutex();
    
    if (!m_commandQueue || !m_stateMutex) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS objects");
        deinitialize();
        return false;
    }

    // Create protocol handling task
    if (xTaskCreate(taskFunction, "ProtocolHandler", StackSizes::PROTOCOL_HANDLER, 
                    this, TaskPriorities::PROTOCOL_HANDLER, &m_taskHandle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create protocol handler task");
        deinitialize();
        return false;
    }

    m_initialized = true;
    m_deviceState = DeviceState::ONLINE;
    
    ESP_LOGI(TAG, "Protocol handler initialized (Address: 0x%02X)", m_deviceAddress);
    return true;
}

void ProtocolHandler::deinitialize() {
    if (!m_initialized) return;

    if (m_taskHandle) {
        vTaskDelete(m_taskHandle);
        m_taskHandle = nullptr;
    }

    if (m_commandQueue) {
        vQueueDelete(m_commandQueue);
        m_commandQueue = nullptr;
    }

    if (m_stateMutex) {
        vSemaphoreDelete(m_stateMutex);
        m_stateMutex = nullptr;
    }

    m_initialized = false;
    m_deviceState = DeviceState::OFFLINE;
    
    ESP_LOGI(TAG, "Protocol handler deinitialized");
}

void ProtocolHandler::setDeviceState(DeviceState state) {
    if (xSemaphoreTake(m_stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        m_deviceState = state;
        xSemaphoreGive(m_stateMutex);
    }
}

void ProtocolHandler::setSwitchState(bool isOn) {
    if (xSemaphoreTake(m_stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        bool changed = (m_switchState != isOn);
        m_switchState = isOn;
        xSemaphoreGive(m_stateMutex);
        
        if (changed) {
            ESP_LOGI(TAG, "Switch state changed to: %s", isOn ? "ON" : "OFF");
        }
    }
}

// Static task function
void ProtocolHandler::taskFunction(void* parameter) {
    ProtocolHandler* instance = static_cast<ProtocolHandler*>(parameter);
    instance->handleProtocol();
}

void ProtocolHandler::handleProtocol() {
    SlaveRS485::Message rxMessage;
    
    while (true) {
        // Process incoming messages from RS485
        while (m_rs485.receiveMessage(rxMessage, 0)) {
            uint8_t sourceAddress;
            CommandType cmdType = parseMessage(rxMessage, sourceAddress);
            
            switch (cmdType) {
                case CommandType::PING:
                    handlePingCommand(sourceAddress);
                    break;
                    
                case CommandType::SWITCH_ON:
                case CommandType::SWITCH_OFF:
                    handleSwitchCommand(cmdType);
                    break;
                    
                case CommandType::UNKNOWN:
                    m_errorCount++;
                    logMessage(rxMessage, "Unknown command");
                    break;
            }
        }
        
        // Small delay to prevent tight looping
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

ProtocolHandler::CommandType ProtocolHandler::parseMessage(const SlaveRS485::Message& message, uint8_t& sourceAddress) {
    if (message.length < 2) {
        return CommandType::UNKNOWN;
    }
    
    // Check for ping message: [OurAddress] [0x00]
    if (isPingMessage(message, sourceAddress)) {
        return CommandType::PING;
    }
    
    // Check for command message
    CommandType cmdType;
    if (isCommandMessage(message, cmdType)) {
        return cmdType;
    }
    
    return CommandType::UNKNOWN;
}

bool ProtocolHandler::isPingMessage(const SlaveRS485::Message& message, uint8_t& sourceAddress) {
    if (message.length == 2 && 
        message.data[0] == m_deviceAddress && 
        message.data[1] == 0x00) {
        
        sourceAddress = 0x00; // Master
        return true;
    }
    
    return false;
}

bool ProtocolHandler::isCommandMessage(const SlaveRS485::Message& message, CommandType& cmdType) {
    // This would implement parsing of more complex command messages
    // For now, simplified based on your original code structure
    
    if (message.length >= 5 && message.data[0] == m_deviceAddress) {
        // Check for command structure similar to your original code
        if (message.data[1] == 0x03) { // Command type
            if (message.data[4] == 0x00) {
                cmdType = CommandType::SWITCH_ON;
                return true;
            } else if (message.data[4] == 0x80) {
                cmdType = CommandType::SWITCH_OFF;
                return true;
            }
        }
    }
    
    return false;
}

void ProtocolHandler::handlePingCommand(uint8_t sourceAddress) {
    m_pingCount++;
    
    ESP_LOGD(TAG, "Ping received from 0x%02X", sourceAddress);
    
    // Send ping response
    auto result = m_rs485.sendPingResponse();
    if (result != SlaveRS485::TransmitResult::SUCCESS) {
        m_errorCount++;
        ESP_LOGW(TAG, "Failed to send ping response");
    }
}

void ProtocolHandler::handleSwitchCommand(CommandType cmdType) {
    m_commandCount++;
    
    bool newState = (cmdType == CommandType::SWITCH_ON);
    setSwitchState(newState);
    
    ESP_LOGI(TAG, "Switch command: %s", newState ? "ON" : "OFF");
    
    // Send command response
    auto result = m_rs485.sendCommandResponse(newState);
    if (result != SlaveRS485::TransmitResult::SUCCESS) {
        m_errorCount++;
        ESP_LOGW(TAG, "Failed to send command response");
    }
}

void ProtocolHandler::logMessage(const SlaveRS485::Message& message, const char* description) {
    #if defined(DEBUG_PROTOCOL)
    ESP_LOGD(TAG, "%s - Length: %d, Data: ", description, message.length);
    for (size_t i = 0; i < message.length && i < 16; i++) {
        ESP_LOGD(TAG, "0x%02X ", message.data[i]);
    }
    #endif
}