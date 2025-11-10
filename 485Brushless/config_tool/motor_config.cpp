#include <M5Unified.h>
#include <Wire.h>

// Unit Roller485 I2C Configuration
#define MOTOR_I2C_ADDR 0x64
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

void setup() {
    // Initialize M5Unified
    M5.begin();
    Serial.begin(115200);
    
    // Initialize I2C on Port A (G21=SDA, G22=SCL for M5Stack Station 485)
    Wire.begin(21, 22);
    
    Serial.println("Unit Roller485 Configuration Tool");
    Serial.println("Current mode: I2C at address 0x64");
    
    delay(1000);
    
    // Read current configuration
    Serial.println("\nReading current configuration...");
    uint8_t currentMode = readRegister(REG_COMM_MODE);
    uint8_t currentID = readRegister(REG_RS485_ID);
    uint8_t currentBaud = readRegister(REG_BAUD_RATE);
    
    Serial.printf("Current mode: %s\n", currentMode == MODE_I2C ? "I2C" : "RS485");
    Serial.printf("Current RS485 ID: %d\n", currentID);
    Serial.printf("Current baud rate code: %d\n", currentBaud);
    
    // Configure for RS485 mode
    Serial.println("\nConfiguring for RS485 mode...");
    
    // Set RS485 mode
    if (writeRegister(REG_COMM_MODE, MODE_RS485)) {
        Serial.println("✓ Set communication mode to RS485");
    } else {
        Serial.println("✗ Failed to set RS485 mode");
        return;
    }
    
    // Set RS485 ID to 1
    if (writeRegister(REG_RS485_ID, 1)) {
        Serial.println("✓ Set RS485 ID to 1");
    } else {
        Serial.println("✗ Failed to set RS485 ID");
        return;
    }
    
    // Set baud rate to 115200
    if (writeRegister(REG_BAUD_RATE, BAUD_115200)) {
        Serial.println("✓ Set baud rate to 115200");
    } else {
        Serial.println("✗ Failed to set baud rate");
        return;
    }
    
    // Save configuration to flash
    if (writeRegister(REG_SAVE_CONFIG, 0x01)) {
        Serial.println("✓ Configuration saved to flash");
    } else {
        Serial.println("✗ Failed to save configuration");
        return;
    }
    
    Serial.println("\n🎉 Unit Roller485 configured for RS485 mode!");
    Serial.println("Settings:");
    Serial.println("- Mode: RS485");
    Serial.println("- ID: 1");
    Serial.println("- Baud Rate: 115200");
    Serial.println("\nPlease power cycle the Unit Roller485 to apply changes.");
}

void loop() {
    // Nothing to do in loop
    delay(1000);
}

bool writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MOTOR_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    uint8_t result = Wire.endTransmission();
    delay(10); // Give the motor time to process
    return (result == 0);
}

uint8_t readRegister(uint8_t reg) {
    Wire.beginTransmission(MOTOR_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0xFF; // Error reading
    }
    
    Wire.requestFrom(MOTOR_I2C_ADDR, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF; // Error reading
}