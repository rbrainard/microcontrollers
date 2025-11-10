#include <Arduino.h>
#include <M5Stack.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "config.h"
#include "RS485Communication.h"
#include "SlaveManager.h"
#include "UI.h"

static const char* TAG = "CrestronMaster";

// Global objects
RS485Communication g_rs485;
SlaveManager g_slaveManager(g_rs485);

// UI state
volatile bool g_pingEnabled = false;
volatile bool g_dimRequest1 = false;
volatile bool g_dimRequest2 = false;

// Function declarations
void setupTasks();
void uiTaskFunction(void* parameter);
void statusTaskFunction(void* parameter);
void handleButtons();
void updateDisplay();

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    ESP_LOGI(TAG, "Starting Crestron Master Emulator");
    ESP_LOGI(TAG, "Build: %s %s", __DATE__, __TIME__);
    
    // Initialize M5Stack
    M5.begin(true, false, true);
    M5.Power.begin();
    
    // Initialize UI
    UI::begin();
    UI::showStartup();
    
    // Initialize RS485 communication
    if (!g_rs485.initialize()) {
        ESP_LOGE(TAG, "Failed to initialize RS485");
        M5.Lcd.setTextColor(RED);
        M5.Lcd.println("RS485 FAILED!");
        while (true) { delay(1000); }
    }
    
    // Initialize slave manager
    if (!g_slaveManager.initialize()) {
        ESP_LOGE(TAG, "Failed to initialize slave manager");
        M5.Lcd.setTextColor(RED);
        M5.Lcd.println("SlaveManager FAILED!");
        while (true) { delay(1000); }
    }
    
    // Add known slaves
    g_slaveManager.addSlave(SlaveDevices::IO_48_ADDRESS, SlaveManager::SlaveType::IO_48);
    g_slaveManager.addSlave(SlaveDevices::DIM8_ADDRESS, SlaveManager::SlaveType::DIM8);
    g_slaveManager.addSlave(SlaveDevices::DIMU8_ADDRESS, SlaveManager::SlaveType::DIMU8);
    
    // Setup additional tasks
    setupTasks();
    
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("Ready!");
    
    ESP_LOGI(TAG, "Initialization complete");
    ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
    
    delay(1000);
    // initial display update
    const uint8_t slaves[] = { SlaveDevices::IO_48_ADDRESS, SlaveDevices::DIM8_ADDRESS, SlaveDevices::DIMU8_ADDRESS };
    const char* slaveNames[] = { "IO-48", "DIM8 ", "DIMU8" };
    UI::updateMainScreen(g_pingEnabled, esp_get_free_heap_size(), g_rs485.getTransmitCount(), g_rs485.getReceiveCount(), g_rs485.getErrorCount(), g_slaveManager.getSuccessfulPings(), g_slaveManager.getTotalPings(), slaves, slaveNames, 3, g_dimRequest1, g_dimRequest2);
}

void loop() {
    // Main loop is minimal - most work done in FreeRTOS tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Watchdog-style status check
    static uint32_t lastStatusCheck = 0;
    if (millis() - lastStatusCheck > 5000) {
        ESP_LOGI(TAG, "System status - Free heap: %d, RS485 TX: %d, RX: %d", 
                 esp_get_free_heap_size(),
                 g_rs485.getTransmitCount(),
                 g_rs485.getReceiveCount());
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
        // Update M5Stack and handle buttons
        M5.update();
        handleButtons();
        
        // Update display if needed
        static TickType_t lastDisplayUpdate = 0;
        if (xTaskGetTickCount() - lastDisplayUpdate > pdMS_TO_TICKS(250)) {
            const uint8_t slaves[] = { SlaveDevices::IO_48_ADDRESS, SlaveDevices::DIM8_ADDRESS, SlaveDevices::DIMU8_ADDRESS };
            const char* slaveNames[] = { "IO-48", "DIM8 ", "DIMU8" };
            UI::updateMainScreen(g_pingEnabled, esp_get_free_heap_size(), g_rs485.getTransmitCount(), g_rs485.getReceiveCount(), g_rs485.getErrorCount(), g_slaveManager.getSuccessfulPings(), g_slaveManager.getTotalPings(), slaves, slaveNames, 3, g_dimRequest1, g_dimRequest2);
            lastDisplayUpdate = xTaskGetTickCount();
        }
        
        // Run at 50Hz for responsive UI
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(20));
    }
}

void statusTaskFunction(void* parameter) {
    while (true) {
        // Monitor system health
        uint32_t freeHeap = esp_get_free_heap_size();
        if (freeHeap < 10000) {
            ESP_LOGW(TAG, "Low memory warning: %d bytes free", freeHeap);
        }
        
        // Check RS485 error rates
        uint32_t totalMessages = g_rs485.getTransmitCount() + g_rs485.getReceiveCount();
        uint32_t errorCount = g_rs485.getErrorCount();
        
        if (totalMessages > 100 && (errorCount * 100 / totalMessages) > 5) {
            ESP_LOGW(TAG, "High RS485 error rate: %d%% (%d/%d)", 
                     (errorCount * 100 / totalMessages), errorCount, totalMessages);
        }
        
        // Check slave connectivity
        auto onlineSlaves = g_slaveManager.getOnlineSlaves();
        ESP_LOGD(TAG, "Online slaves: %d", onlineSlaves.size());
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void handleButtons() {
    // Button A: Toggle pinging
    if (M5.BtnA.wasReleased()) {
        g_pingEnabled = !g_pingEnabled;
        g_slaveManager.enablePinging(g_pingEnabled);
        
        ESP_LOGI(TAG, "Pinging %s", g_pingEnabled ? "enabled" : "disabled");
    }
    
    // Button B: DIM commands
    if (M5.BtnB.wasReleased()) {
        g_dimRequest1 = !g_dimRequest1;
        
        if (g_dimRequest1) {
            // Send DIM command to turn on channel 1 at full brightness
            g_slaveManager.sendDimCommand(SlaveDevices::DIM8_ADDRESS, 1, 255, 500);
            g_slaveManager.sendDimUCommand(SlaveDevices::DIMU8_ADDRESS, 1, 255, 500);
            ESP_LOGI(TAG, "Dim ON commands sent");
        } else {
            // Send DIM command to turn off channel 1
            g_slaveManager.sendDimCommand(SlaveDevices::DIM8_ADDRESS, 1, 0, 500);
            g_slaveManager.sendDimUCommand(SlaveDevices::DIMU8_ADDRESS, 1, 0, 500);
            ESP_LOGI(TAG, "Dim OFF commands sent");
        }
    }
    
    // Button C: Secondary DIM commands
    if (M5.BtnC.wasReleased()) {
        g_dimRequest2 = !g_dimRequest2;
        
        if (g_dimRequest2) {
            // Send DIM command to turn on channel 2 at 50% brightness
            g_slaveManager.sendDimCommand(SlaveDevices::DIM8_ADDRESS, 2, 128, 1000);
            g_slaveManager.sendDimUCommand(SlaveDevices::DIMU8_ADDRESS, 2, 128, 1000);
            ESP_LOGI(TAG, "Dim2 ON commands sent");
        } else {
            // Send DIM command to turn off channel 2
            g_slaveManager.sendDimCommand(SlaveDevices::DIM8_ADDRESS, 2, 0, 1000);
            g_slaveManager.sendDimUCommand(SlaveDevices::DIMU8_ADDRESS, 2, 0, 1000);
            ESP_LOGI(TAG, "Dim2 OFF commands sent");
        }
    }
}

void updateDisplay() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(1);
    
    // Title
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Crestron Master");
    
    // Status line
    M5.Lcd.setTextSize(1);
    M5.Lcd.printf("Ping: %s  Heap: %d\n", 
                  g_pingEnabled ? "ON " : "OFF", 
                  esp_get_free_heap_size());
    
    // Communication stats
    M5.Lcd.printf("TX: %d  RX: %d  ERR: %d\n",
                  g_rs485.getTransmitCount(),
                  g_rs485.getReceiveCount(),
                  g_rs485.getErrorCount());
    
    // Ping statistics
    uint32_t totalPings = g_slaveManager.getTotalPings();
    uint32_t successPings = g_slaveManager.getSuccessfulPings();
    uint32_t pingRate = totalPings > 0 ? (successPings * 100 / totalPings) : 0;
    
    M5.Lcd.printf("Pings: %d/%d (%d%%)\n", successPings, totalPings, pingRate);
    
    // Slave status
    M5.Lcd.println("\nSlave Status:");
    
    const uint8_t slaves[] = {
        SlaveDevices::IO_48_ADDRESS,
        SlaveDevices::DIM8_ADDRESS,
        SlaveDevices::DIMU8_ADDRESS
    };
    
    const char* slaveNames[] = {"IO-48", "DIM8 ", "DIMU8"};
    
    for (int i = 0; i < 3; i++) {
        auto state = g_slaveManager.getSlaveState(slaves[i]);
        const char* stateStr = "OFFLINE";
        uint16_t color = RED;
        
        switch (state) {
            case SlaveManager::SlaveState::ONLINE:
                stateStr = "ONLINE ";
                color = GREEN;
                break;
            case SlaveManager::SlaveState::CONFIGURED:
                stateStr = "CONFIG ";
                color = BLUE;
                break;
            case SlaveManager::SlaveState::PING_SENT:
                stateStr = "PING.. ";
                color = YELLOW;
                break;
            case SlaveManager::SlaveState::CONFIGURING:
                stateStr = "SETUP  ";
                color = ORANGE;
                break;
            default:
                break;
        }
        
        M5.Lcd.setTextColor(color);
        M5.Lcd.printf("0x%02X %s %s\n", slaves[i], slaveNames[i], stateStr);
    }
    
    // Button labels
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(0, 220);
    M5.Lcd.printf("A:Ping  B:Dim1%s  C:Dim2%s",
                  g_dimRequest1 ? "*" : " ",
                  g_dimRequest2 ? "*" : " ");
}