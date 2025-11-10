#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/timers.h>
#include <map>
#include <vector>
#include "config.h"
#include "RS485Communication.h"

/**
 * Manages communication with multiple Crestron slave devices
 * Handles configuration sequences, status monitoring, and command dispatch
 */
class SlaveManager {
public:
    enum class SlaveType {
        IO_48,      // 48-channel I/O module
        DIM8,       // 8-channel dimmer
        DIMU8       // 8-channel universal dimmer
    };
    
    enum class SlaveState {
        OFFLINE,
        PING_SENT,
        ONLINE,
        CONFIG_REQUESTED,
        CONFIGURING,
        CONFIGURED,
        ERROR
    };
    
    enum class ConfigStep {
        NONE = 0,
        INIT_SYNC,
        TIME_SYNC,
        CHANNEL_CONFIG,
        LOAD_CONFIG,
        FINALIZE,
        COMPLETE
    };

    struct SlaveInfo {
        uint8_t address;
        SlaveType type;
        SlaveState state;
        ConfigStep configStep;
        uint32_t lastPingTime;
        uint32_t configStepIndex;
        bool dimRequest1;
        bool dimRequest2;
        uint8_t errorCount;
    };

    SlaveManager(RS485Communication& rs485);
    ~SlaveManager();

    bool initialize();
    void deinitialize();
    
    // Slave management
    bool addSlave(uint8_t address, SlaveType type);
    bool removeSlave(uint8_t address);
    void enablePinging(bool enable);
    
    // Command interface
    bool sendDimCommand(uint8_t address, uint8_t channel, uint8_t level, uint16_t rampTime = 0);
    bool sendDimUCommand(uint8_t address, uint8_t channel, uint8_t level, uint16_t rampTime = 0);
    
    // Status queries
    SlaveState getSlaveState(uint8_t address) const;
    std::vector<uint8_t> getOnlineSlaves() const;
    std::vector<uint8_t> getOfflineSlaves() const;
    
    // Statistics
    uint32_t getTotalPings() const { return m_totalPings; }
    uint32_t getSuccessfulPings() const { return m_successfulPings; }
    uint32_t getConfigurationCount() const { return m_configurationCount; }

private:
    RS485Communication& m_rs485;
    std::map<uint8_t, SlaveInfo> m_slaves;
    
    // FreeRTOS objects
    TaskHandle_t m_taskHandle;
    TimerHandle_t m_pingTimer;
    QueueHandle_t m_commandQueue;
    SemaphoreHandle_t m_slavesMutex;
    
    // State management
    bool m_initialized;
    bool m_pingEnabled;
    uint8_t m_currentSlaveIndex;
    uint32_t m_pingSequenceNumber;
    
    // Statistics
    volatile uint32_t m_totalPings;
    volatile uint32_t m_successfulPings;
    volatile uint32_t m_configurationCount;
    
    // Configuration data
    struct ConfigData {
        SlaveType type;
        std::vector<std::vector<uint8_t>> sequences;
    };
    
    std::map<SlaveType, ConfigData> m_configTemplates;
    
    // Internal command structure
    struct Command {
        enum Type {
            PING,
            DIM_COMMAND,
            DIMU_COMMAND,
            CONFIG_STEP
        } type;
        
        uint8_t address;
        uint8_t channel;
        uint8_t level;
        uint16_t rampTime;
        std::vector<uint8_t> data;
    };
    
    // Task functions
    static void taskFunction(void* parameter);
    static void pingTimerCallback(TimerHandle_t timer);
    
    // Internal methods
    void handleTask();
    void processPingCycle();
    void processConfigurationStep(uint8_t address);
    void handleIncomingMessage(const RS485Communication::Message& message);
    
    // Configuration helpers
    void initializeConfigTemplates();
    bool sendConfigurationSequence(uint8_t address, ConfigStep step);
    void advanceConfigurationStep(uint8_t address);
    
    // Communication helpers
    bool sendPingToSlave(uint8_t address);
    bool sendTimeSync(uint8_t address);
    void updateSlaveState(uint8_t address, SlaveState newState);
    
    // Timing and scheduling
    void scheduleNextPing();
    bool isPingTimeout(const SlaveInfo& slave) const;
    void handlePingTimeout(uint8_t address);
};