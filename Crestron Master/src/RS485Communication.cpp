#include "RS485Communication.h"
#include <esp_log.h>
#include <soc/uart_reg.h>
#include <driver/gpio.h>

static const char* TAG = "RS485";

RS485Communication::RS485Communication() 
    : m_initialized(false)
    , m_rxQueue(nullptr)
    , m_txQueue(nullptr)
    , m_txMutex(nullptr)
    , m_rxTaskHandle(nullptr)
    , m_txTaskHandle(nullptr)
    , m_transmitCount(0)
    , m_receiveCount(0)
    , m_errorCount(0)
{
}

RS485Communication::~RS485Communication() {
    deinitialize();
}

bool RS485Communication::initialize() {
    if (m_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    // Create FreeRTOS objects
    m_rxQueue = xQueueCreate(CrestronProtocol::MESSAGE_QUEUE_SIZE, sizeof(Message));
    m_txQueue = xQueueCreate(CrestronProtocol::MESSAGE_QUEUE_SIZE, sizeof(Message));
    m_txMutex = xSemaphoreCreateMutex();

    if (!m_rxQueue || !m_txQueue || !m_txMutex) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS objects");
        deinitialize();
        return false;
    }

    // Configure UART
    if (!configureUART()) {
        ESP_LOGE(TAG, "Failed to configure UART");
        deinitialize();
        return false;
    }

    // Configure DE/RE pin for RS485 transceiver
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RS485Config::DE_RE_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure DE/RE pin");
        deinitialize();
        return false;
    }

    // Set to receive mode initially
    setTransmitMode(false);

    // Start communication tasks
    startTasks();

    m_initialized = true;
    ESP_LOGI(TAG, "RS485 communication initialized successfully");
    return true;
}

void RS485Communication::deinitialize() {
    if (!m_initialized) return;

    stopTasks();

    if (m_rxQueue) {
        vQueueDelete(m_rxQueue);
        m_rxQueue = nullptr;
    }
    
    if (m_txQueue) {
        vQueueDelete(m_txQueue);
        m_txQueue = nullptr;
    }
    
    if (m_txMutex) {
        vSemaphoreDelete(m_txMutex);
        m_txMutex = nullptr;
    }

    uart_driver_delete(UART_PORT);
    m_initialized = false;
    
    ESP_LOGI(TAG, "RS485 communication deinitialized");
}

bool RS485Communication::configureUART() {
    uart_config_t uart_config = {
        .baud_rate = static_cast<int>(RS485Config::BAUD_RATE),
        .data_bits = RS485Config::DATA_BITS,
        .parity = RS485Config::PARITY,
        .stop_bits = RS485Config::STOP_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_APB
    };

    if (uart_param_config(UART_PORT, &uart_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters");
        return false;
    }

    if (uart_set_pin(UART_PORT, RS485Config::TX_PIN, RS485Config::RX_PIN, 
                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins");
        return false;
    }

    if (uart_driver_install(UART_PORT, RX_BUFFER_SIZE, TX_BUFFER_SIZE, 
                           0, nullptr, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver");
        return false;
    }

    return true;
}

void RS485Communication::startTasks() {
    xTaskCreate(rxTaskFunction, "RS485_RX", StackSizes::RS485_HANDLER, 
                this, TaskPriorities::RS485_HANDLER, &m_rxTaskHandle);
                
    xTaskCreate(txTaskFunction, "RS485_TX", StackSizes::RS485_HANDLER, 
                this, TaskPriorities::RS485_HANDLER, &m_txTaskHandle);
}

void RS485Communication::stopTasks() {
    if (m_rxTaskHandle) {
        vTaskDelete(m_rxTaskHandle);
        m_rxTaskHandle = nullptr;
    }
    
    if (m_txTaskHandle) {
        vTaskDelete(m_txTaskHandle);
        m_txTaskHandle = nullptr;
    }
}

bool RS485Communication::sendMessage(const uint8_t* data, size_t length) {
    if (!m_initialized || !data || length == 0 || length > CrestronProtocol::MAX_MESSAGE_LENGTH) {
        return false;
    }

    Message message;
    memcpy(message.data, data, length);
    message.length = length;
    message.timestamp = xTaskGetTickCount();
    message.isIncoming = false;

    if (xSemaphoreTake(m_txMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        bool result = xQueueSend(m_txQueue, &message, pdMS_TO_TICKS(50)) == pdTRUE;
        xSemaphoreGive(m_txMutex);
        return result;
    }
    
    return false;
}

bool RS485Communication::sendPing(uint8_t slaveAddress) {
    uint8_t pingData[2] = {slaveAddress, CrestronProtocol::PING_COMMAND};
    return sendMessage(pingData, sizeof(pingData));
}

bool RS485Communication::sendBreak() {
    // Send a break signal by pulling TX low for precise timing
    setTransmitMode(true);
    
    // Critical section for precise timing
    noInterrupts();
    gpio_set_level(static_cast<gpio_num_t>(RS485Config::TX_PIN), 0);
    criticalSectionDelay(CrestronTiming::BREAK_DURATION_US);
    gpio_set_level(static_cast<gpio_num_t>(RS485Config::TX_PIN), 1);
    interrupts();
    
    vTaskDelay(pdMS_TO_TICKS(1)); // Brief recovery time
    setTransmitMode(false);
    
    return true;
}

bool RS485Communication::receiveMessage(Message& message, TickType_t timeout) {
    if (!m_initialized) return false;
    
    return xQueueReceive(m_rxQueue, &message, timeout) == pdTRUE;
}

size_t RS485Communication::getAvailableMessages() const {
    return m_rxQueue ? uxQueueMessagesWaiting(m_rxQueue) : 0;
}

void RS485Communication::setTransmitMode(bool enable) {
    gpio_set_level(static_cast<gpio_num_t>(RS485Config::DE_RE_PIN), enable ? 1 : 0);
    
    // Small delay to allow transceiver to switch modes
    if (enable) {
        delayMicroseconds(10); // TX enable delay
    } else {
        delayMicroseconds(5);  // RX enable delay
    }
}

void RS485Communication::flushBuffers() {
    uart_flush(UART_PORT);
    
    // Clear our queues
    if (m_rxQueue) {
        xQueueReset(m_rxQueue);
    }
    if (m_txQueue) {
        xQueueReset(m_txQueue);
    }
}

// Static task functions
void RS485Communication::rxTaskFunction(void* parameter) {
    RS485Communication* instance = static_cast<RS485Communication*>(parameter);
    instance->handleReceive();
}

void RS485Communication::txTaskFunction(void* parameter) {
    RS485Communication* instance = static_cast<RS485Communication*>(parameter);
    instance->handleTransmit();
}

void RS485Communication::handleReceive() {
    Message message;
    uint8_t buffer[CrestronProtocol::MAX_MESSAGE_LENGTH];
    
    while (true) {
        int length = uart_read_bytes(UART_PORT, buffer, sizeof(buffer), pdMS_TO_TICKS(100));
        
        if (length > 0) {
            message.length = length;
            memcpy(message.data, buffer, length);
            message.timestamp = xTaskGetTickCount();
            message.isIncoming = true;
            
            if (xQueueSend(m_rxQueue, &message, 0) == pdTRUE) {
                m_receiveCount++;
            } else {
                m_errorCount++;
                ESP_LOGW(TAG, "RX queue full, dropping message");
            }
        }
    }
}

void RS485Communication::handleTransmit() {
    Message message;
    
    while (true) {
        if (xQueueReceive(m_txQueue, &message, portMAX_DELAY) == pdTRUE) {
            setTransmitMode(true);
            
            int bytesWritten = uart_write_bytes(UART_PORT, message.data, message.length);
            
            // Wait for transmission to complete
            uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100));
            
            setTransmitMode(false);
            
            if (bytesWritten == message.length) {
                m_transmitCount++;
            } else {
                m_errorCount++;
                ESP_LOGW(TAG, "Failed to transmit complete message");
            }
        }
    }
}

void RS485Communication::preciseDelay(uint32_t microseconds) {
    if (microseconds < 1000) {
        delayMicroseconds(microseconds);
    } else {
        vTaskDelay(pdMS_TO_TICKS(microseconds / 1000));
    }
}

void RS485Communication::criticalSectionDelay(uint32_t microseconds) {
    // Use ESP32 high-resolution timer for critical timing
    uint64_t start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) < microseconds) {
        // Busy wait for precise timing
    }
}