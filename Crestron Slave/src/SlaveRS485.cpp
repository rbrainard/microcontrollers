#include "SlaveRS485.h"
#include <esp_log.h>
#include <driver/gpio.h>
#include <soc/uart_reg.h>

static const char* TAG = "SlaveRS485";

SlaveRS485::SlaveRS485()
    : m_initialized(false)
    , m_rxQueue(nullptr)
    , m_txMutex(nullptr)
    , m_rxTaskHandle(nullptr)
    , m_receiveCount(0)
    , m_transmitCount(0)
    , m_errorCount(0)
{
}

SlaveRS485::~SlaveRS485() {
    deinitialize();
}

bool SlaveRS485::initialize() {
    if (m_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    // Create FreeRTOS objects
    m_rxQueue = xQueueCreate(Protocol::MESSAGE_QUEUE_SIZE, sizeof(Message));
    m_txMutex = xSemaphoreCreateMutex();

    if (!m_rxQueue || !m_txMutex) {
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

    // Configure RS485 direction control pin
    if (!configureRS485Pin()) {
        ESP_LOGE(TAG, "Failed to configure RS485 pin");
        deinitialize();
        return false;
    }

    // Set to receive mode initially
    setTransmitMode(false);

    // Start receive task
    startReceiveTask();

    m_initialized = true;
    ESP_LOGI(TAG, "Slave RS485 initialized successfully");
    return true;
}

void SlaveRS485::deinitialize() {
    if (!m_initialized) return;

    stopReceiveTask();

    if (m_rxQueue) {
        vQueueDelete(m_rxQueue);
        m_rxQueue = nullptr;
    }
    
    if (m_txMutex) {
        vSemaphoreDelete(m_txMutex);
        m_txMutex = nullptr;
    }

    uart_driver_delete(UART_PORT);
    m_initialized = false;
    
    ESP_LOGI(TAG, "Slave RS485 deinitialized");
}

bool SlaveRS485::configureUART() {
    uart_config_t uart_config = {
        .baud_rate = static_cast<int>(SlaveConfig::BAUD_RATE),
        .data_bits = SlaveConfig::DATA_BITS,
        .parity = SlaveConfig::PARITY,
        .stop_bits = SlaveConfig::STOP_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_APB
    };

    if (uart_param_config(UART_PORT, &uart_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters");
        return false;
    }

    if (uart_set_pin(UART_PORT, SlaveConfig::RS485_TX_PIN, SlaveConfig::RS485_RX_PIN, 
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

bool SlaveRS485::configureRS485Pin() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SlaveConfig::RS485_DE_RE_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    return gpio_config(&io_conf) == ESP_OK;
}

void SlaveRS485::startReceiveTask() {
    xTaskCreate(rxTaskFunction, "SlaveRS485_RX", StackSizes::COMMUNICATION, 
                this, TaskPriorities::COMMUNICATION, &m_rxTaskHandle);
}

void SlaveRS485::stopReceiveTask() {
    if (m_rxTaskHandle) {
        vTaskDelete(m_rxTaskHandle);
        m_rxTaskHandle = nullptr;
    }
}

bool SlaveRS485::receiveMessage(Message& message, TickType_t timeout) {
    if (!m_initialized) return false;
    
    return xQueueReceive(m_rxQueue, &message, timeout) == pdTRUE;
}

size_t SlaveRS485::getAvailableMessages() const {
    return m_rxQueue ? uxQueueMessagesWaiting(m_rxQueue) : 0;
}

SlaveRS485::TransmitResult SlaveRS485::sendPingResponse() {
    uint8_t response[2] = {Protocol::TO_MASTER_PREFIX, Protocol::PING_RESPONSE};
    return sendMessage(response, sizeof(response));
}

SlaveRS485::TransmitResult SlaveRS485::sendCommandResponse(bool isOn) {
    uint8_t response[5] = {
        Protocol::TO_MASTER_PREFIX, 
        Protocol::COMMAND_RESPONSE, 
        0x00, 
        0x00, 
        isOn ? Protocol::ON_COMMAND : Protocol::OFF_COMMAND
    };
    return sendMessage(response, sizeof(response));
}

SlaveRS485::TransmitResult SlaveRS485::sendMessage(const uint8_t* data, size_t length) {
    if (!m_initialized || !data || length == 0 || length > Protocol::MAX_MESSAGE_LENGTH) {
        return TransmitResult::ERROR;
    }

    if (xSemaphoreTake(m_txMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        transmitWithTiming(data, length);
        xSemaphoreGive(m_txMutex);
        
        m_transmitCount++;
        return TransmitResult::SUCCESS;
    }
    
    m_errorCount++;
    return TransmitResult::TIMEOUT;
}

void SlaveRS485::setTransmitMode(bool enable) {
    gpio_set_level(static_cast<gpio_num_t>(SlaveConfig::RS485_DE_RE_PIN), enable ? 1 : 0);
}

void SlaveRS485::transmitWithTiming(const uint8_t* data, size_t length) {
    // Pre-transmit delay
    preciseDelayMicroseconds(ProtocolTiming::PRE_TRANSMIT_DELAY_US);
    
    // Enable transmitter
    setTransmitMode(true);
    
    // Wait for transmitter to be ready
    preciseDelayMicroseconds(ProtocolTiming::TRANSMIT_ENABLE_DELAY_US);
    
    // Send the data
    int bytesWritten = uart_write_bytes(UART_PORT, data, length);
    
    // Wait for transmission to complete
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(50));
    
    // Post-transmit delay before disabling transmitter
    preciseDelayMicroseconds(ProtocolTiming::POST_TRANSMIT_DELAY_US);
    
    // Disable transmitter (enable receiver)
    setTransmitMode(false);
    
    if (bytesWritten != length) {
        ESP_LOGW(TAG, "Incomplete transmission: %d/%d bytes", bytesWritten, length);
        m_errorCount++;
    }
}

void SlaveRS485::flushBuffers() {
    uart_flush(UART_PORT);
    
    if (m_rxQueue) {
        xQueueReset(m_rxQueue);
    }
}

// Static task function
void SlaveRS485::rxTaskFunction(void* parameter) {
    SlaveRS485* instance = static_cast<SlaveRS485*>(parameter);
    instance->handleReceive();
}

void SlaveRS485::handleReceive() {
    Message message;
    uint8_t buffer[Protocol::MAX_MESSAGE_LENGTH];
    
    while (true) {
        int length = uart_read_bytes(UART_PORT, buffer, sizeof(buffer), pdMS_TO_TICKS(100));
        
        if (length > 0) {
            message.length = length;
            memcpy(message.data, buffer, length);
            message.timestamp = xTaskGetTickCount();
            message.isIncoming = true;
            
            if (xQueueSend(m_rxQueue, &message, 0) == pdTRUE) {
                m_receiveCount++;
                ESP_LOGD(TAG, "Received %d bytes", length);
            } else {
                m_errorCount++;
                ESP_LOGW(TAG, "RX queue full, dropping message");
            }
        }
    }
}

void SlaveRS485::preciseDelayMicroseconds(uint32_t microseconds) {
    if (microseconds < 1000) {
        delayMicroseconds(microseconds);
    } else {
        vTaskDelay(pdMS_TO_TICKS(microseconds / 1000));
    }
}