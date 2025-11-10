#include <Arduino.h>
#include <M5Stack.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "config.h"
#include "SlaveRS485.h"
#include "ProtocolHandler.h"
#include "SwitchHandler.h"

static const char* TAG = "CrestronSlave";

// Global objects
SlaveRS485 g_rs485;
ProtocolHandler g_protocolHandler(g_rs485);
SwitchHandler g_switchHandler;

// Function declarations
void setupTasks();
void uiTaskFunction(void* parameter);
void statusTaskFunction(void* parameter);
void updateDisplay();
void switchEventCallback(SwitchHandler::SwitchEvent event, bool currentState);

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    ESP_LOGI(TAG, "Starting Crestron Slave Device");
    ESP_LOGI(TAG, "Device Address: 0x%02X", SlaveConfig::DEVICE_ADDRESS);
    ESP_LOGI(TAG, "Build: %s %s", __DATE__, __TIME__);
    
    // Initialize M5Stack
    M5.begin(true, false, true);
    M5.Power.begin();
    
    // Clear display and show startup message
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("Crestron Slave");
    M5.Lcd.setTextSize(1);
    M5.Lcd.printf("Address: 0x%02X\n", SlaveConfig::DEVICE_ADDRESS);
    M5.Lcd.println("Initializing...");
    
    // Initialize RS485 communication
    if (!g_rs485.initialize()) {
        ESP_LOGE(TAG, "Failed to initialize RS485");
        M5.Lcd.setTextColor(RED);
        M5.Lcd.println("RS485 FAILED!");
        while (true) { delay(1000); }
    }
    
    // Initialize protocol handler
    if (!g_protocolHandler.initialize()) {
        ESP_LOGE(TAG, "Failed to initialize protocol handler");
        M5.Lcd.setTextColor(RED);
        M5.Lcd.println("Protocol FAILED!");
        while (true) { delay(1000); }
    }
    
    // Initialize switch handler
    if (!g_switchHandler.initialize(&g_protocolHandler)) {
        ESP_LOGE(TAG, "Failed to initialize switch handler");
        M5.Lcd.setTextColor(RED);
        M5.Lcd.println("Switch FAILED!");
        while (true) { delay(1000); }
    }
    
    // Set switch event callback
    g_switchHandler.setEventCallback(switchEventCallback);
    
    // Setup additional tasks
    setupTasks();
    
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("Ready!");
    
    ESP_LOGI(TAG, "Initialization complete");
    ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
    
    delay(1000);
    updateDisplay();
}

void loop() {
    // Main loop is minimal - most work done in FreeRTOS tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Watchdog-style status check
    static uint32_t lastStatusCheck = 0;
    if (millis() - lastStatusCheck > 10000) {
        ESP_LOGI(TAG, "Status - Heap: %d, RX: %d, TX: %d, Pings: %d", 
                 esp_get_free_heap_size(),
                 g_rs485.getReceiveCount(),
                 g_rs485.getTransmitCount(),
                 g_protocolHandler.getPingCount());
        lastStatusCheck = millis();
    }
}

void setupTasks() {
    // Create UI handling task
    xTaskCreate(uiTaskFunction, "UI_Handler", StackSizes::UI_HANDLER, 
                nullptr, TaskPriorities::UI_HANDLER, nullptr);
    
    // Create status monitoring task
    xTaskCreate(statusTaskFunction, "Status_Monitor", StackSizes::STATUS_MONITOR, 
                nullptr, TaskPriorities::STATUS_MONITOR, nullptr);
}

void uiTaskFunction(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    while (true) {
        // Update M5Stack (this handles button reading internally)
        M5.update();
        
        // Update display periodically
        static TickType_t lastDisplayUpdate = 0;
        if (xTaskGetTickCount() - lastDisplayUpdate > pdMS_TO_TICKS(500)) {
            updateDisplay();
            lastDisplayUpdate = xTaskGetTickCount();
        }
        
        // Run at 20Hz for responsive UI
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(50));
    }
}

void statusTaskFunction(void* parameter) {
    while (true) {
        // Monitor system health
        uint32_t freeHeap = esp_get_free_heap_size();
        if (freeHeap < 10000) {
            ESP_LOGW(TAG, "Low memory warning: %d bytes free", freeHeap);
        }
        
        // Check communication health
        static uint32_t lastRxCount = 0;
        static uint32_t lastTxCount = 0;
        uint32_t currentRx = g_rs485.getReceiveCount();
        uint32_t currentTx = g_rs485.getTransmitCount();
        
        if (currentRx == lastRxCount && currentTx == lastTxCount) {
            ESP_LOGD(TAG, "No communication activity detected");
        }
        
        lastRxCount = currentRx;
        lastTxCount = currentTx;
        
        // Check error rates
        uint32_t errorCount = g_rs485.getErrorCount() + g_protocolHandler.getErrorCount();
        if (errorCount > 0) {
            ESP_LOGW(TAG, "Communication errors detected: %d", errorCount);
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void updateDisplay() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(1);
    
    // Title and device info
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("Slave 0x%02X\n", SlaveConfig::DEVICE_ADDRESS);
    
    // Device state
    M5.Lcd.setTextSize(1);
    auto deviceState = g_protocolHandler.getDeviceState();
    const char* stateStr = "OFFLINE";
    uint16_t stateColor = RED;
    
    switch (deviceState) {
        case ProtocolHandler::DeviceState::ONLINE:
            stateStr = "ONLINE";
            stateColor = GREEN;
            break;
        case ProtocolHandler::DeviceState::PROCESSING_COMMAND:
            stateStr = "PROCESSING";
            stateColor = YELLOW;
            break;
        default:
            break;
    }
    
    M5.Lcd.setTextColor(stateColor);
    M5.Lcd.printf("State: %s\n", stateStr);
    
    // Switch state
    M5.Lcd.setTextColor(WHITE);
    bool switchState = g_switchHandler.getSwitchState();
    M5.Lcd.setTextColor(switchState ? GREEN : RED);
    M5.Lcd.printf("Switch: %s\n", switchState ? "ON" : "OFF");
    
    // Communication statistics
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.printf("RX: %d  TX: %d\n", 
                  g_rs485.getReceiveCount(),
                  g_rs485.getTransmitCount());
    
    M5.Lcd.printf("Pings: %d  Cmds: %d\n",
                  g_protocolHandler.getPingCount(),
                  g_protocolHandler.getCommandCount());
    
    M5.Lcd.printf("Errors: %d\n",
                  g_rs485.getErrorCount() + g_protocolHandler.getErrorCount());
    
    // System info
    M5.Lcd.printf("Heap: %d bytes\n", esp_get_free_heap_size());
    M5.Lcd.printf("Uptime: %d sec\n", millis() / 1000);
    
    // Switch statistics
    M5.Lcd.println("\nSwitch Stats:");
    M5.Lcd.printf("Presses: %d\n", g_switchHandler.getPressCount());
    M5.Lcd.printf("Long: %d  Double: %d\n",
                  g_switchHandler.getLongPressCount(),
                  g_switchHandler.getDoubleClickCount());
    
    // Button labels
    M5.Lcd.setCursor(0, 220);
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.printf("A:Switch  B:---  C:---");
    
    // Connection status indicator
    M5.Lcd.fillCircle(310, 10, 8, 
                      deviceState == ProtocolHandler::DeviceState::ONLINE ? GREEN : RED);
}

void switchEventCallback(SwitchHandler::SwitchEvent event, bool currentState) {
    const char* eventStr = "UNKNOWN";
    
    switch (event) {
        case SwitchHandler::SwitchEvent::PRESS:
            eventStr = "PRESS";
            break;
        case SwitchHandler::SwitchEvent::RELEASE:
            eventStr = "RELEASE";
            break;
        case SwitchHandler::SwitchEvent::LONG_PRESS:
            eventStr = "LONG_PRESS";
            break;
        case SwitchHandler::SwitchEvent::DOUBLE_CLICK:
            eventStr = "DOUBLE_CLICK";
            break;
    }
    
    ESP_LOGI(TAG, "Switch event: %s, State: %s", 
             eventStr, currentState ? "ON" : "OFF");
}