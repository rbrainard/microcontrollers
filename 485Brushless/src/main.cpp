#include <M5Unified.h>
#include "M5PWR485MotorController.h"
#include "MotorWebServer.h"
#include <unit_rolleri2c.hpp>
#include <Wire.h>

// Configuration
#define MOTOR_ID 1
#define WIFI_SSID "Hacienda"
#define WIFI_PASSWORD "brainframe"
#define WEB_SERVER_PORT 80

// Pin assignments for M5Stack Station 485
// PWR485 uses the built-in RS485 transceiver (Serial2)
#define RS485_RX_PIN 16   // PWR485 RX (GPIO16/U2RXD)
#define RS485_TX_PIN 17   // PWR485 TX (GPIO17/U2TXD) 
#define RS485_DE_PIN 19   // PWR485 DE (GPIO19) - Direction Enable

// I2C Configuration for Unit Roller485 on Port A2
#define MOTOR_I2C_ADDR 0x01  // Motor is configured for I2C address 1
#define I2C_SDA_PIN 32  // Port A2 SDA (GPIO32)
#define I2C_SCL_PIN 33  // Port A2 SCL (GPIO33)

// Unit Roller485 I2C Registers
#define REG_COMM_MODE 0x00  // Communication mode register
#define REG_RS485_ID 0x01   // RS485 ID register
#define REG_BAUD_RATE 0x02  // Baud rate register
#define REG_SAVE_CONFIG 0xFF // Save configuration register

// Communication modes
#define MODE_I2C 0x00
#define MODE_RS485 0x01

// Baud rates
#define BAUD_9600 0x00
#define BAUD_19200 0x01
#define BAUD_57600 0x02
#define BAUD_115200 0x03

// Global state variables
bool i2cInitialized = false;
bool motorConfigured = false;
bool useI2CMode = false; // True if using I2C, false if using RS485

// Global objects
UnitRoller485Controller motorController(MOTOR_ID);
UnitRollerI2C motorControllerI2C;  // M5Stack official library class
MotorWebServer webServer(&motorController);

// Export pointers for web server access
UnitRollerI2C* g_motorI2C = &motorControllerI2C;
bool* g_useI2CMode = &useI2CMode;

// Function prototypes
void handleButtons();
void handleCenterButton();
void executeControlCommand();
void updateDisplay();
void displayStatusScreen();
void displayControlScreen();
void displayConfigScreen();
void displayNetworkScreen();

// I2C Configuration functions
bool configureMotorRS485Mode();
bool writeI2CRegister(uint8_t reg, uint8_t value);
uint8_t readI2CRegister(uint8_t reg);
void displayI2CConfigScreen();
bool initializeI2C();
void scanI2CDevices();

// Display variables
uint32_t lastDisplayUpdate = 0;
uint32_t lastStatusUpdate = 0;
const uint32_t DISPLAY_UPDATE_INTERVAL = 100;  // 100ms
const uint32_t STATUS_UPDATE_INTERVAL = 500;   // 500ms

// Current display mode
enum DisplayMode {
    DISPLAY_STATUS,
    DISPLAY_CONTROL,
    DISPLAY_CONFIG,
    DISPLAY_NETWORK,
    DISPLAY_I2C_CONFIG
};

DisplayMode currentDisplayMode = DISPLAY_STATUS;
bool motorEnabled = false;

// Control variables
float targetSpeed = 0.0;
int32_t targetPosition = 0;
float targetTorque = 0.0;
MotorMode controlMode = MODE_DISABLE;

// Network status
bool wifiConnected = false;
IPAddress currentIP;

void setup() {
    // Initialize M5Unified for Station 485
    auto cfg = M5.config();
    cfg.clear_display = true;
    cfg.output_power = true;
    cfg.internal_imu = false;  // Station 485 doesn't have IMU
    cfg.internal_rtc = true;   // Station 485 has RTC
    cfg.internal_spk = false;  // Station 485 doesn't have speaker
    cfg.internal_mic = false;  // Station 485 doesn't have microphone
    M5.begin(cfg);
    
    Serial.begin(115200);
    Serial.println("M5Stack Station 485 - Unit Roller485 Controller Starting...");
    
    // Initialize display
    M5.Display.setRotation(1);
    M5.Display.setTextSize(2);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(WHITE);
    M5.Display.drawString("Initializing...", 10, 10);
    
    // Initialize motor controller with correct baud rate for Unit Roller485
    Serial.println("Initializing Unit Roller485 controller...");
    Serial.printf("Using pins - RX:%d, TX:%d, DE:%d\n", RS485_RX_PIN, RS485_TX_PIN, RS485_DE_PIN);
    Serial.printf("Motor ID: %d, Baud Rate: 115200\n", MOTOR_ID);
    
    // First, scan for I2C devices to check if motor is in I2C mode
    Serial.println("Scanning I2C bus for devices...");
    scanI2CDevices();
    
    // If motor detected in I2C mode, initialize I2C control
    if (i2cInitialized) {
        Serial.println("Attempting to initialize I2C motor control...");
        // Use M5Stack official library - begin(TwoWire, addr, sda, scl, speed)
        if (motorControllerI2C.begin(&Wire, MOTOR_I2C_ADDR, I2C_SDA_PIN, I2C_SCL_PIN, 100000)) {
            Serial.println("I2C motor control initialized successfully");
            useI2CMode = true;
            M5.Display.drawString("Motor: I2C Mode", 10, 40);
            
            // Try to read some data to verify communication
            Serial.println("Testing I2C communication with motor...");
            int32_t speed = motorControllerI2C.getSpeed();  // Returns value*100 (e.g., 10000 for 100 RPM)
            int32_t position = motorControllerI2C.getPosReadback();
            int32_t voltage = motorControllerI2C.getVin();  // Returns value*100 (e.g., 1200 for 12.0V)
            Serial.printf("Initial values - Speed: %d (%.1f RPM), Position: %d, Voltage: %d (%.2f V)\n", 
                         speed, speed/100.0f, position, voltage, voltage/100.0f);
        } else {
            Serial.println("Failed to initialize I2C motor control");
            Serial.println("Motor not responding at address 0x01");
            M5.Display.drawString("Motor: I2C FAILED", 10, 40);
        }
    } else {
        Serial.println("No motor detected in I2C mode, trying RS485...");
    }
    
    // Try RS485 initialization if not in I2C mode
    if (!useI2CMode) {
        if (motorController.begin(&Serial2, 115200, RS485_RX_PIN, RS485_TX_PIN, RS485_DE_PIN)) {
            Serial.println("Unit Roller485 controller initialized successfully");
            M5.Display.drawString("Motor: OK", 10, 40);
            
            // Try to update status and show debug info
            Serial.println("Testing motor communication...");
            if (motorController.updateStatus()) {
                Serial.println("Motor status update successful");
            } else {
                Serial.println("Motor status update failed - check wiring and motor power");
            }
        } else {
            Serial.println("Failed to initialize Unit Roller485 controller");
            Serial.println("Check:");
            Serial.println("1. Unit Roller485 is powered (6-16V on PWR485 connector)");
            Serial.println("2. RS485 connections are correct");
            Serial.println("3. Motor ID is set to 1");
            Serial.println("4. Baud rate is 115200");
            Serial.println("5. Motor may be in I2C mode - use I2C CONFIG screen to switch to RS485");
            M5.Display.drawString("Motor: FAILED", 10, 40);
        }
    }
    
    // Initialize WiFi and web server
    Serial.println("Starting WiFi connection...");
    M5.Display.drawString("WiFi: Connecting...", 10, 70);
    
    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        currentIP = WiFi.localIP();
        Serial.println("WiFi connected successfully");
        Serial.print("IP Address: ");
        Serial.println(currentIP);
        M5.Display.drawString("WiFi: Connected", 10, 70);
        M5.Display.drawString("IP: " + currentIP.toString(), 10, 100);
        
        // Start web server
        webServer.begin();
    } else {
        Serial.println("Failed to connect to WiFi, starting AP mode");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("M5-Roller485-Controller", "12345678");
        wifiConnected = true;
        currentIP = WiFi.softAPIP();
        M5.Display.drawString("WiFi: AP Mode", 10, 70);
        M5.Display.drawString("IP: " + currentIP.toString(), 10, 100);
        
        // Start web server
        webServer.begin();
    }
    
    delay(2000);
    
    // Initialize motor configuration for Unit Roller485
    MotorConfig config;
    config.motorId = MOTOR_ID;
    config.maxVelocity = 2000.0;    // Max RPM for Unit Roller485
    config.acceleration = 500.0;    // RPM/s
    config.maxCurrent = 500.0;      // mA (0.5A continuous)
    config.enableLimits = false;
    config.minPosition = 0;
    config.maxPosition = 36000;     // 360 degrees in encoder counts
    config.kp_velocity = 100;
    config.ki_velocity = 50;
    config.kd_velocity = 10;
    config.kp_position = 200;
    config.ki_position = 100;
    config.kd_position = 20;
    
    // Only set motor config if using RS485 mode
    if (!useI2CMode) {
        motorController.setMotorConfig(config);
    }
    
    Serial.println("Setup complete");
}

void loop() {
    M5.update();
    
    uint32_t currentTime = millis();
    
    // Update motor status periodically
    if (currentTime - lastStatusUpdate >= STATUS_UPDATE_INTERVAL) {
        bool statusUpdated = false;
        if (useI2CMode) {
            // For I2C mode, status is always available
            statusUpdated = true;
        } else {
            statusUpdated = motorController.updateStatus();
        }
        
        if (!statusUpdated) {
            static uint32_t lastErrorPrint = 0;
            // Print error every 10 seconds to avoid spam
            if (currentTime - lastErrorPrint >= 10000) {
                Serial.println("Warning: Motor status update failed");
                lastErrorPrint = currentTime;
            }
        }
        lastStatusUpdate = currentTime;
    }
    
    // Handle button presses
    handleButtons();
    
    // Update display
    if (currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        updateDisplay();
        lastDisplayUpdate = currentTime;
    }
    
    // Handle emergency stop via button combination
    if (M5.BtnA.wasPressed() && M5.BtnB.wasPressed()) {
        motorController.disableMotor();
        Serial.println("Emergency stop activated via buttons");
    }
    
    delay(10);
}

void handleButtons() {
    // Button A (Left) - Previous display mode
    if (M5.BtnA.wasPressed()) {
        currentDisplayMode = (DisplayMode)((currentDisplayMode - 1 + 5) % 5);
    }
    
    // Button B (Center) - Action button
    if (M5.BtnB.wasPressed()) {
        handleCenterButton();
    }
    
    // Button C (Right) - Next display mode
    if (M5.BtnC.wasPressed()) {
        currentDisplayMode = (DisplayMode)((currentDisplayMode + 1) % 5);
    }
}

void handleCenterButton() {
    switch (currentDisplayMode) {
        case DISPLAY_STATUS:
            // Toggle motor enable/disable
            motorEnabled = !motorEnabled;
            if (motorEnabled) {
                motorController.enableMotor();
            } else {
                motorController.disableMotor();
            }
            break;
            
        case DISPLAY_CONTROL:
            // Execute current control command
            executeControlCommand();
            break;
            
        case DISPLAY_CONFIG:
            // Reset position
            motorController.resetPosition();
            break;
            
        case DISPLAY_NETWORK:
            // Reset position as default action
            motorController.resetPosition();
            break;
            
        case DISPLAY_I2C_CONFIG:
            // Configure motor for RS485 mode or scan for I2C devices
            if (!i2cInitialized) {
                Serial.println("Rescanning I2C bus for devices...");
                scanI2CDevices();
            } else {
                if (configureMotorRS485Mode()) {
                    motorConfigured = true;
                    Serial.println("Motor configured successfully!");
                    Serial.println("Please power cycle the Unit Roller485 to activate RS485 mode");
                }
            }
            break;
    }
}

void executeControlCommand() {
    switch (controlMode) {
        case MODE_VELOCITY:
            motorController.setVelocityMode(targetSpeed);
            break;
            
        case MODE_POSITION:
            motorController.setPositionMode(targetPosition, 1000);
            break;
            
        case MODE_CURRENT:
            motorController.setCurrentMode(targetTorque);
            break;
            
        case MODE_DISABLE:
            motorController.disableMotor();
            break;
    }
}

void updateDisplay() {
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(1);
    
    // Display header
    M5.Display.drawString("Unit Roller485 Controller", 10, 5);
    
    // Display mode indicator
    String modeStr = "";
    switch (currentDisplayMode) {
        case DISPLAY_STATUS: modeStr = "STATUS"; break;
        case DISPLAY_CONTROL: modeStr = "CONTROL"; break;
        case DISPLAY_CONFIG: modeStr = "CONFIG"; break;
        case DISPLAY_NETWORK: modeStr = "NETWORK"; break;
        case DISPLAY_I2C_CONFIG: modeStr = "I2C CONFIG"; break;
    }
    M5.Display.drawString("Mode: " + modeStr, 10, 20);
    
    // Display content based on current mode
    switch (currentDisplayMode) {
        case DISPLAY_STATUS:
            displayStatusScreen();
            break;
            
        case DISPLAY_CONTROL:
            displayControlScreen();
            break;
            
        case DISPLAY_CONFIG:
            displayConfigScreen();
            break;
            
        case DISPLAY_NETWORK:
            displayNetworkScreen();
            break;
            
        case DISPLAY_I2C_CONFIG:
            displayI2CConfigScreen();
            break;
    }
    
    // Display button labels
    M5.Display.setTextColor(YELLOW);
    M5.Display.drawString("PREV", 5, 220);
    M5.Display.drawString("ACTION", 140, 220);
    M5.Display.drawString("NEXT", 280, 220);
}

void displayStatusScreen() {
    MotorStatus status = motorController.getStatus();
    
    M5.Display.setTextColor(WHITE);
    M5.Display.drawString("Unit Roller485 Status:", 10, 40);
    
    // Running status
    M5.Display.setTextColor(status.isRunning ? GREEN : RED);
    M5.Display.drawString("Running: " + String(status.isRunning ? "YES" : "NO"), 10, 60);
    
    // Mode
    M5.Display.setTextColor(WHITE);
    String mode = "";
    switch (status.currentMode) {
        case MODE_VELOCITY: mode = "Velocity"; break;
        case MODE_POSITION: mode = "Position"; break;
        case MODE_CURRENT: mode = "Current"; break;
        case MODE_DISABLE: mode = "Disabled"; break;
    }
    M5.Display.drawString("Mode: " + mode, 10, 80);
    
    // Values
    M5.Display.drawString("Velocity: " + String(status.velocity, 1) + " RPM", 10, 100);
    M5.Display.drawString("Position: " + String(status.position) + " (" + 
                        String(motorController.positionToDegrees(status.position), 1) + "°)", 10, 120);
    M5.Display.drawString("Current: " + String(status.current, 2) + "A", 10, 140);
    M5.Display.drawString("Voltage: " + String(status.voltage, 1) + "V", 10, 160);
    M5.Display.drawString("Temp: " + String(status.temperature) + "C", 200, 160);
    M5.Display.drawString("Encoder: " + String(status.encoderReady ? "OK" : "ERR"), 200, 180);
    
    // Error status
    if (status.errorCode != 0) {
        M5.Display.setTextColor(RED);
        M5.Display.drawString("Error: " + motorController.getErrorString(status.errorCode), 10, 200);
    }
}

void displayControlScreen() {
    M5.Display.setTextColor(WHITE);
    M5.Display.drawString("Motor Control:", 10, 40);
    
    // Control mode selection (simulated - would need encoder or additional buttons)
    M5.Display.drawString("Control Mode:", 10, 60);
    
    String modeStr = "";
    switch (controlMode) {
        case MODE_VELOCITY: modeStr = "Velocity Control"; break;
        case MODE_POSITION: modeStr = "Position Control"; break;
        case MODE_CURRENT: modeStr = "Current Control"; break;
        case MODE_DISABLE: modeStr = "Motor Disabled"; break;
    }
    M5.Display.setTextColor(CYAN);
    M5.Display.drawString(modeStr, 10, 80);
    
    // Target values
    M5.Display.setTextColor(WHITE);
    M5.Display.drawString("Target Values:", 10, 100);
    M5.Display.drawString("Velocity: " + String(targetSpeed) + " RPM", 10, 120);
    M5.Display.drawString("Position: " + String(targetPosition) + " (" + 
                        String(motorController.positionToDegrees(targetPosition), 1) + "°)", 10, 140);
    M5.Display.drawString("Current: " + String(targetTorque) + " mA", 10, 160);
    
    M5.Display.setTextColor(GREEN);
    M5.Display.drawString("Press ACTION to execute", 10, 180);
}

void displayConfigScreen() {
    MotorConfig config = motorController.getMotorConfig();
    
    M5.Display.setTextColor(WHITE);
    M5.Display.drawString("Motor Configuration:", 10, 40);
    
    M5.Display.drawString("Motor ID: " + String(config.motorId), 10, 60);
    M5.Display.drawString("Max Velocity: " + String(config.maxVelocity) + " RPM", 10, 80);
    M5.Display.drawString("Acceleration: " + String(config.acceleration), 10, 100);
    M5.Display.drawString("Max Current: " + String(config.maxCurrent) + " A", 10, 120);
    M5.Display.drawString("Position Limits: " + String(config.enableLimits ? "ON" : "OFF"), 10, 140);
    
    if (config.enableLimits) {
        M5.Display.drawString("Min Pos: " + String(config.minPosition), 10, 160);
        M5.Display.drawString("Max Pos: " + String(config.maxPosition), 10, 180);
    }
    
    M5.Display.setTextColor(GREEN);
    M5.Display.drawString("Press ACTION to reset position", 10, 200);
}

void displayNetworkScreen() {
    M5.Display.setTextColor(WHITE);
    M5.Display.drawString("Network Status:", 10, 40);
    
    // WiFi status
    M5.Display.setTextColor(wifiConnected ? GREEN : RED);
    M5.Display.drawString("WiFi: " + String(wifiConnected ? "Connected" : "Disconnected"), 10, 60);
    
    if (wifiConnected) {
        M5.Display.setTextColor(WHITE);
        M5.Display.drawString("IP: " + currentIP.toString(), 10, 80);
        M5.Display.drawString("Web Server: Port " + String(WEB_SERVER_PORT), 10, 100);
        M5.Display.drawString("Access via browser:", 10, 120);
        M5.Display.setTextColor(CYAN);
        M5.Display.drawString("http://" + currentIP.toString(), 10, 140);
    }
    
    // Connection info
    M5.Display.setTextColor(WHITE);
    M5.Display.drawString("Connection Info:", 10, 160);
    String wifiStatus = wifiConnected ? ("IP: " + currentIP.toString()) : "Disconnected";
    M5.Display.drawString(wifiStatus, 10, 180);
    
    M5.Display.setTextColor(GREEN);
    M5.Display.drawString("Press ACTION to clear errors", 10, 200);
}

// I2C Configuration Functions
bool initializeI2C() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000); // 100kHz I2C speed
    
    // Test communication with motor
    Wire.beginTransmission(MOTOR_I2C_ADDR);
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
        Serial.println("I2C communication with Unit Roller485 established");
        return true;
    } else {
        Serial.printf("I2C communication failed with error: %d\n", error);
        return false;
    }
}

uint8_t readI2CRegister(uint8_t reg) {
    Wire.beginTransmission(MOTOR_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    
    Wire.requestFrom(MOTOR_I2C_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF; // Error value
}

bool writeI2CRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MOTOR_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
        delay(10); // Small delay for register write
        return true;
    }
    return false;
}

bool configureMotorRS485Mode() {
    Serial.println("Configuring Unit Roller485 for RS485 mode...");
    
    if (!initializeI2C()) {
        Serial.println("Failed to initialize I2C communication");
        return false;
    }
    
    // Read current configuration
    uint8_t currentMode = readI2CRegister(REG_COMM_MODE);
    uint8_t currentID = readI2CRegister(REG_RS485_ID);
    uint8_t currentBaud = readI2CRegister(REG_BAUD_RATE);
    
    Serial.printf("Current config - Mode: 0x%02X, ID: %d, Baud: 0x%02X\n", 
                  currentMode, currentID, currentBaud);
    
    bool configChanged = false;
    
    // Set communication mode to RS485
    if (currentMode != MODE_RS485) {
        if (writeI2CRegister(REG_COMM_MODE, MODE_RS485)) {
            Serial.println("Set communication mode to RS485");
            configChanged = true;
        } else {
            Serial.println("Failed to set communication mode");
            return false;
        }
    }
    
    // Set RS485 ID to 1
    if (currentID != MOTOR_ID) {
        if (writeI2CRegister(REG_RS485_ID, MOTOR_ID)) {
            Serial.printf("Set RS485 ID to %d\n", MOTOR_ID);
            configChanged = true;
        } else {
            Serial.println("Failed to set RS485 ID");
            return false;
        }
    }
    
    // Set baud rate to 115200
    if (currentBaud != BAUD_115200) {
        if (writeI2CRegister(REG_BAUD_RATE, BAUD_115200)) {
            Serial.println("Set baud rate to 115200");
            configChanged = true;
        } else {
            Serial.println("Failed to set baud rate");
            return false;
        }
    }
    
    // Save configuration if any changes were made
    if (configChanged) {
        if (writeI2CRegister(REG_SAVE_CONFIG, 0x01)) {
            Serial.println("Configuration saved to motor");
            delay(500); // Wait for save operation
            
            // Verify configuration
            currentMode = readI2CRegister(REG_COMM_MODE);
            currentID = readI2CRegister(REG_RS485_ID);
            currentBaud = readI2CRegister(REG_BAUD_RATE);
            
            Serial.printf("Verified config - Mode: 0x%02X, ID: %d, Baud: 0x%02X\n", 
                          currentMode, currentID, currentBaud);
            
            if (currentMode == MODE_RS485 && currentID == MOTOR_ID && currentBaud == BAUD_115200) {
                Serial.println("Motor successfully configured for RS485 mode");
                Serial.println("Please power cycle the motor to activate RS485 communication");
                return true;
            } else {
                Serial.println("Configuration verification failed");
                return false;
            }
        } else {
            Serial.println("Failed to save configuration");
            return false;
        }
    } else {
        Serial.println("Motor already configured for RS485 mode");
        return true;
    }
}

void displayI2CConfigScreen() {
    M5.Display.setTextColor(WHITE);
    M5.Display.drawString("I2C Motor Config:", 10, 40);
    
    M5.Display.drawString("Unit Roller485 I2C Setup", 10, 60);
    M5.Display.drawString("I2C Address: 0x" + String(MOTOR_I2C_ADDR, HEX), 10, 80);
    M5.Display.drawString("SDA: GPIO32 (A1-Yellow)", 10, 95);
    M5.Display.drawString("SCL: GPIO33 (A1-White)", 10, 110);
    
    if (i2cInitialized) {
        M5.Display.setTextColor(GREEN);
        M5.Display.drawString("I2C: Motor Detected!", 10, 125);
        
        // Read current configuration
        uint8_t mode = readI2CRegister(REG_COMM_MODE);
        uint8_t id = readI2CRegister(REG_RS485_ID);
        uint8_t baud = readI2CRegister(REG_BAUD_RATE);
        
        M5.Display.setTextColor(WHITE);
        M5.Display.drawString("Current Config:", 10, 140);
        M5.Display.drawString("Mode: " + String(mode == MODE_RS485 ? "RS485" : "I2C"), 10, 155);
        M5.Display.drawString("ID: " + String(id), 10, 170);
        M5.Display.drawString("Baud: " + String(baud == BAUD_115200 ? "115200" : "Other"), 10, 185);
        
        if (mode == MODE_RS485 && id == MOTOR_ID && baud == BAUD_115200) {
            M5.Display.setTextColor(GREEN);
            M5.Display.drawString("Config: Ready for RS485", 10, 200);
        } else {
            M5.Display.setTextColor(YELLOW);
            M5.Display.drawString("Press ACTION to configure", 10, 200);
        }
    } else {
        M5.Display.setTextColor(RED);
        M5.Display.drawString("I2C: No Motor Found", 10, 125);
        M5.Display.setTextColor(YELLOW);
        M5.Display.drawString("Press ACTION to scan again", 10, 200);
    }
}

void scanI2CDevices() {
    // Initialize I2C for scanning
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000); // 100kHz I2C speed
    
    Serial.println("I2C Scanner");
    Serial.printf("Scanning I2C bus on SDA=%d, SCL=%d\n", I2C_SDA_PIN, I2C_SCL_PIN);
    
    int deviceCount = 0;
    bool motorFound = false;
    
    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.printf("I2C device found at address 0x%02X (decimal %d)", address, address);
            if (address == MOTOR_I2C_ADDR) {
                Serial.print(" <-- Unit Roller485 Motor!");
                motorFound = true;
            }
            Serial.println();
            deviceCount++;
        } else if (error == 4) {
            Serial.printf("Unknown error at address 0x%02X\n", address);
        }
    }
    
    // Check if motor is at expected address
    if (motorFound) {
        i2cInitialized = true;
        Serial.println("Unit Roller485 detected in I2C mode at address 0x64!");
        Serial.println("Initializing I2C motor control...");
    } else if (deviceCount > 0) {
        Serial.printf("Found %d I2C device(s) but none at expected motor address 0x64\n", deviceCount);
        Serial.println("The Unit Roller485 default I2C address should be 0x64 (100 decimal)");
        Serial.println("Current device might be at wrong address or not the motor");
    } else {
        Serial.println("No I2C devices found");
        Serial.println("Check:");
        Serial.println("1. Unit Roller485 power (6-16V)");
        Serial.println("2. I2C connections on PORT.A1");
        Serial.println("3. SDA=GPIO32(Yellow), SCL=GPIO33(White)");
    }
    Serial.println("I2C scan complete\n");
}