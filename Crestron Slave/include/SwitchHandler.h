#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include "config.h"

// Forward declaration
class ProtocolHandler;

/**
 * Handles physical switch input with proper debouncing and state management
 * Integrates with ProtocolHandler for coordinated switch state updates
 */
class SwitchHandler {
public:
    enum class SwitchEvent {
        PRESS,
        RELEASE,
        LONG_PRESS,
        DOUBLE_CLICK
    };

    using SwitchCallback = void(*)(SwitchEvent event, bool currentState);

    SwitchHandler();
    ~SwitchHandler();

    bool initialize(ProtocolHandler* protocolHandler);
    void deinitialize();
    
    // State queries
    bool getSwitchState() const { return m_currentState; }
    bool isPressed() const { return m_isPressed; }
    
    // Event handling
    void setEventCallback(SwitchCallback callback);
    
    // Statistics
    uint32_t getPressCount() const { return m_pressCount; }
    uint32_t getLongPressCount() const { return m_longPressCount; }
    uint32_t getDoubleClickCount() const { return m_doubleClickCount; }

private:
    ProtocolHandler* m_protocolHandler;
    TaskHandle_t m_taskHandle;
    TimerHandle_t m_debounceTimer;
    TimerHandle_t m_longPressTimer;
    TimerHandle_t m_doubleClickTimer;
    
    // Switch state
    bool m_initialized;
    bool m_currentState;        // Logical switch state (ON/OFF)
    bool m_isPressed;          // Physical button state
    bool m_lastPhysicalState;
    uint32_t m_lastPressTime;
    uint32_t m_lastReleaseTime;
    bool m_pendingDoubleClick;
    
    // Event callback
    SwitchCallback m_eventCallback;
    
    // Statistics
    volatile uint32_t m_pressCount;
    volatile uint32_t m_longPressCount;
    volatile uint32_t m_doubleClickCount;
    
    // Task and timer functions
    static void taskFunction(void* parameter);
    static void debounceTimerCallback(TimerHandle_t timer);
    static void longPressTimerCallback(TimerHandle_t timer);
    static void doubleClickTimerCallback(TimerHandle_t timer);
    
    // Internal methods
    void handleSwitchInput();
    void processPhysicalStateChange(bool newPhysicalState);
    void handlePress();
    void handleRelease();
    void handleLongPress();
    void handleDoubleClick();
    void toggleSwitchState();
    void updateProtocolHandler();
    
    // Hardware interface
    bool readPhysicalSwitch();
    void configureGPIO();
};