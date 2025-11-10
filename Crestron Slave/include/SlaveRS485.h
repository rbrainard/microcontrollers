#pragma once

#include <Arduino.h>
#include <driver/uart.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "config.h"

/**
 * RS485 communication handler for Crestron slave device
 * Handles precise timing for RS485 transceiver control and message processing
 */
class SlaveRS485 {
public:
    struct Message {
        uint8_t data[Protocol::MAX_MESSAGE_LENGTH];
        size_t length;
        uint32_t timestamp;
        bool isIncoming;
    };

    enum class TransmitResult {
        SUCCESS,
        TIMEOUT,
        ERROR
    };

    SlaveRS485();
    ~SlaveRS485();

    bool initialize();
    void deinitialize();
    
    // Reception
    bool receiveMessage(Message& message, TickType_t timeout = portMAX_DELAY);
    size_t getAvailableMessages() const;
    
    // Transmission with proper RS485 timing
    TransmitResult sendPingResponse();
    TransmitResult sendCommandResponse(bool isOn);
    TransmitResult sendMessage(const uint8_t* data, size_t length);
    
    // Status and diagnostics
    bool isInitialized() const { return m_initialized; }
    uint32_t getReceiveCount() const { return m_receiveCount; }
    uint32_t getTransmitCount() const { return m_transmitCount; }
    uint32_t getErrorCount() const { return m_errorCount; }
    
    // Low-level control
    void flushBuffers();

private:
    static constexpr uart_port_t UART_PORT = UART_NUM_2;
    static constexpr size_t RX_BUFFER_SIZE = 512;
    static constexpr size_t TX_BUFFER_SIZE = 256;
    
    bool m_initialized;
    QueueHandle_t m_rxQueue;
    SemaphoreHandle_t m_txMutex;
    TaskHandle_t m_rxTaskHandle;
    
    // Statistics
    volatile uint32_t m_receiveCount;
    volatile uint32_t m_transmitCount;
    volatile uint32_t m_errorCount;
    
    // Internal methods
    bool configureUART();
    bool configureRS485Pin();
    void startReceiveTask();
    void stopReceiveTask();
    
    // RS485 transceiver control
    void setTransmitMode(bool enable);
    void transmitWithTiming(const uint8_t* data, size_t length);
    
    // Task functions
    static void rxTaskFunction(void* parameter);
    void handleReceive();
    
    // Timing utilities
    void preciseDelayMicroseconds(uint32_t microseconds);
};