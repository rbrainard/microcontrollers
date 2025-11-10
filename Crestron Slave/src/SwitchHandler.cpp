#include "SwitchHandler.h"
#include "ProtocolHandler.h"
#include <M5Stack.h>
#include <esp_log.h>
#include <driver/gpio.h>

static const char* TAG = "SwitchHandler";

SwitchHandler::SwitchHandler()
    : m_protocolHandler(nullptr)
    , m_taskHandle(nullptr)
    , m_debounceTimer(nullptr)
    , m_longPressTimer(nullptr)
    , m_doubleClickTimer(nullptr)
    , m_initialized(false)
    , m_currentState(false)
    , m_isPressed(false)
    , m_lastPhysicalState(false)
    , m_lastPressTime(0)
    , m_lastReleaseTime(0)
    , m_pendingDoubleClick(false)
    , m_eventCallback(nullptr)
    , m_pressCount(0)
    , m_longPressCount(0)
    , m_doubleClickCount(0)
{
}

SwitchHandler::~SwitchHandler() {
    deinitialize();
}

bool SwitchHandler::initialize(ProtocolHandler* protocolHandler) {
    if (m_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    if (!protocolHandler) {
        ESP_LOGE(TAG, "Protocol handler is required");
        return false;
    }

    m_protocolHandler = protocolHandler;

    // Configure GPIO (M5Stack Button A is handled by M5 library)
    // No additional GPIO config needed for M5Stack buttons

    // Create timers
    m_debounceTimer = xTimerCreate("SwitchDebounce", 
                                   pdMS_TO_TICKS(ProtocolTiming::DEBOUNCE_DELAY_MS),
                                   pdFALSE, this, debounceTimerCallback);
    
    m_longPressTimer = xTimerCreate("SwitchLongPress", 
                                    pdMS_TO_TICKS(SwitchConfig::LONG_PRESS_MS),
                                    pdFALSE, this, longPressTimerCallback);
    
    m_doubleClickTimer = xTimerCreate("SwitchDoubleClick", 
                                      pdMS_TO_TICKS(300), // 300ms window for double click
                                      pdFALSE, this, doubleClickTimerCallback);

    if (!m_debounceTimer || !m_longPressTimer || !m_doubleClickTimer) {
        ESP_LOGE(TAG, "Failed to create timers");
        deinitialize();
        return false;
    }

    // Create switch handling task
    if (xTaskCreate(taskFunction, "SwitchHandler", StackSizes::SWITCH_HANDLER, 
                    this, TaskPriorities::SWITCH_HANDLER, &m_taskHandle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create switch handler task");
        deinitialize();
        return false;
    }

    m_initialized = true;
    ESP_LOGI(TAG, "Switch handler initialized");
    return true;
}

void SwitchHandler::deinitialize() {
    if (!m_initialized) return;

    if (m_taskHandle) {
        vTaskDelete(m_taskHandle);
        m_taskHandle = nullptr;
    }

    if (m_debounceTimer) {
        xTimerDelete(m_debounceTimer, portMAX_DELAY);
        m_debounceTimer = nullptr;
    }
    
    if (m_longPressTimer) {
        xTimerDelete(m_longPressTimer, portMAX_DELAY);
        m_longPressTimer = nullptr;
    }
    
    if (m_doubleClickTimer) {
        xTimerDelete(m_doubleClickTimer, portMAX_DELAY);
        m_doubleClickTimer = nullptr;
    }

    m_initialized = false;
    ESP_LOGI(TAG, "Switch handler deinitialized");
}

void SwitchHandler::setEventCallback(SwitchCallback callback) {
    m_eventCallback = callback;
}

// Static task function
void SwitchHandler::taskFunction(void* parameter) {
    SwitchHandler* instance = static_cast<SwitchHandler*>(parameter);
    instance->handleSwitchInput();
}

void SwitchHandler::handleSwitchInput() {
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    while (true) {
        // Read current physical switch state
        bool currentPhysicalState = readPhysicalSwitch();
        
        // Check for state change
        if (currentPhysicalState != m_lastPhysicalState) {
            // Start debounce timer
            xTimerReset(m_debounceTimer, 0);
        }
        
        m_lastPhysicalState = currentPhysicalState;
        
        // Run at 20Hz for responsive switch handling
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(50));
    }
}

bool SwitchHandler::readPhysicalSwitch() {
    // Use M5Stack Button A
    M5.update();
    return M5.BtnA.isPressed();
}

// Timer callbacks
void SwitchHandler::debounceTimerCallback(TimerHandle_t timer) {
    SwitchHandler* instance = static_cast<SwitchHandler*>(pvTimerGetTimerID(timer));
    if (instance) {
        bool currentState = instance->readPhysicalSwitch();
        instance->processPhysicalStateChange(currentState);
    }
}

void SwitchHandler::longPressTimerCallback(TimerHandle_t timer) {
    SwitchHandler* instance = static_cast<SwitchHandler*>(pvTimerGetTimerID(timer));
    if (instance) {
        instance->handleLongPress();
    }
}

void SwitchHandler::doubleClickTimerCallback(TimerHandle_t timer) {
    SwitchHandler* instance = static_cast<SwitchHandler*>(pvTimerGetTimerID(timer));
    if (instance) {
        instance->handleDoubleClick();
    }
}

void SwitchHandler::processPhysicalStateChange(bool newPhysicalState) {
    if (newPhysicalState != m_isPressed) {
        m_isPressed = newPhysicalState;
        
        if (newPhysicalState) {
            handlePress();
        } else {
            handleRelease();
        }
    }
}

void SwitchHandler::handlePress() {
    m_lastPressTime = xTaskGetTickCount();
    m_pressCount++;
    
    // Start long press timer
    xTimerStart(m_longPressTimer, 0);
    
    ESP_LOGD(TAG, "Switch pressed");
    
    if (m_eventCallback) {
        m_eventCallback(SwitchEvent::PRESS, m_currentState);
    }
}

void SwitchHandler::handleRelease() {
    m_lastReleaseTime = xTaskGetTickCount();
    
    // Stop long press timer
    xTimerStop(m_longPressTimer, 0);
    
    // Check for double click
    if (m_pendingDoubleClick) {
        // This is the second click of a double click
        m_pendingDoubleClick = false;
        xTimerStop(m_doubleClickTimer, 0);
        
        m_doubleClickCount++;
        ESP_LOGI(TAG, "Double click detected");
        
        if (m_eventCallback) {
            m_eventCallback(SwitchEvent::DOUBLE_CLICK, m_currentState);
        }
    } else {
        // Single click - start double click timer
        m_pendingDoubleClick = true;
        xTimerStart(m_doubleClickTimer, 0);
    }
    
    // Toggle switch state on release (normal behavior)
    toggleSwitchState();
    
    ESP_LOGD(TAG, "Switch released");
    
    if (m_eventCallback) {
        m_eventCallback(SwitchEvent::RELEASE, m_currentState);
    }
}

void SwitchHandler::handleLongPress() {
    m_longPressCount++;
    
    ESP_LOGI(TAG, "Long press detected");
    
    if (m_eventCallback) {
        m_eventCallback(SwitchEvent::LONG_PRESS, m_currentState);
    }
}

void SwitchHandler::handleDoubleClick() {
    // Double click timer expired - process as single click
    if (m_pendingDoubleClick) {
        m_pendingDoubleClick = false;
        // Single click was already processed in handleRelease()
    }
}

void SwitchHandler::toggleSwitchState() {
    m_currentState = !m_currentState;
    updateProtocolHandler();
    
    ESP_LOGI(TAG, "Switch state: %s", m_currentState ? "ON" : "OFF");
}

void SwitchHandler::updateProtocolHandler() {
    if (m_protocolHandler) {
        m_protocolHandler->setSwitchState(m_currentState);
    }
}