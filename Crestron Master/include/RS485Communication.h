#pragma once

#include <Arduino.h>
#include <driver/uart.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "config.h"

/**
 * High-precision RS485 communication class
 * Uses hardware UART for precise timing and FreeRTOS queues for thread-safe communication
 */
class RS485Communication {
public:
    struct Message {
        uint8_t data[CrestronProtocol::MAX_MESSAGE_LENGTH];
        size_t length;
        uint32_t timestamp;
        bool isIncoming;
    };

    RS485Communication();
    ~RS485Communication();

    bool initialize();
    void deinitialize();
    
    // Transmission methods
    bool sendMessage(const uint8_t* data, size_t length);
    bool sendPing(uint8_t slaveAddress);
    bool sendBreak(); // Send zero-bit break signal
    
    // Reception methods
    bool receiveMessage(Message& message, TickType_t timeout = portMAX_DELAY);
    size_t getAvailableMessages() const;
    
    // Status and diagnostics
    bool isInitialized() const { return m_initialized; }
    uint32_t getTransmitCount() const { return m_transmitCount; }
    uint32_t getReceiveCount() const { return m_receiveCount; }
    uint32_t getErrorCount() const { return m_errorCount; }
    
    // Low-level control
    void setTransmitMode(bool enable);
    void flushBuffers();

private:
    static constexpr uart_port_t UART_PORT = UART_NUM_2;
    static constexpr size_t RX_BUFFER_SIZE = 1024;
    static constexpr size_t TX_BUFFER_SIZE = 512;
    
    bool m_initialized;
    QueueHandle_t m_rxQueue;
    QueueHandle_t m_txQueue;
    SemaphoreHandle_t m_txMutex;
    TaskHandle_t m_rxTaskHandle;
    TaskHandle_t m_txTaskHandle;
    
    // Statistics
    volatile uint32_t m_transmitCount;
    volatile uint32_t m_receiveCount;
    volatile uint32_t m_errorCount;
    
    // Internal methods
    bool configureUART();
    void startTasks();
    void stopTasks();
    
    // Static task functions
    static void rxTaskFunction(void* parameter);
    static void txTaskFunction(void* parameter);
    
    // Instance task methods
    void handleReceive();
    void handleTransmit();
    
    // Timing utilities
    void preciseDelay(uint32_t microseconds);
    void criticalSectionDelay(uint32_t microseconds);
};