#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "config.h"
#include "SlaveRS485.h"

/**
 * Handles Crestron protocol parsing and response generation
 * Manages device state and coordinates with switch handler
 */
class ProtocolHandler {
public:
    enum class DeviceState {
        OFFLINE,
        ONLINE,
        PROCESSING_COMMAND
    };
    
    enum class CommandType {
        PING,
        SWITCH_ON,
        SWITCH_OFF,
        UNKNOWN
    };

    struct Command {
        CommandType type;
        uint8_t sourceAddress;
        uint32_t timestamp;
        bool requiresResponse;
    };

    ProtocolHandler(SlaveRS485& rs485);
    ~ProtocolHandler();

    bool initialize();
    void deinitialize();
    
    // State management
    DeviceState getDeviceState() const { return m_deviceState; }
    void setDeviceState(DeviceState state);
    
    // Switch state management
    void setSwitchState(bool isOn);
    bool getSwitchState() const { return m_switchState; }
    
    // Statistics
    uint32_t getCommandCount() const { return m_commandCount; }
    uint32_t getPingCount() const { return m_pingCount; }
    uint32_t getErrorCount() const { return m_errorCount; }

private:
    SlaveRS485& m_rs485;
    TaskHandle_t m_taskHandle;
    QueueHandle_t m_commandQueue;
    SemaphoreHandle_t m_stateMutex;
    
    // Device state
    bool m_initialized;
    DeviceState m_deviceState;
    bool m_switchState;
    uint8_t m_deviceAddress;
    
    // Statistics
    volatile uint32_t m_commandCount;
    volatile uint32_t m_pingCount;
    volatile uint32_t m_errorCount;
    
    // Task function
    static void taskFunction(void* parameter);
    void handleProtocol();
    
    // Message parsing
    CommandType parseMessage(const SlaveRS485::Message& message, uint8_t& sourceAddress);
    bool isPingMessage(const SlaveRS485::Message& message, uint8_t& sourceAddress);
    bool isCommandMessage(const SlaveRS485::Message& message, CommandType& cmdType);
    
    // Response handling
    void handlePingCommand(uint8_t sourceAddress);
    void handleSwitchCommand(CommandType cmdType);
    
    // Utility functions
    void logMessage(const SlaveRS485::Message& message, const char* description);
};