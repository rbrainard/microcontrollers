#ifndef UNIT_ROLLER485_I2C_H
#define UNIT_ROLLER485_I2C_H

#include <Arduino.h>
#include <Wire.h>

// Unit Roller485 I2C Registers
#define I2C_REG_MODE 0x00           // Motor mode register
#define I2C_REG_SPEED 0x10          // Speed control register (2 bytes, signed)
#define I2C_REG_POSITION 0x20       // Position control register (4 bytes, signed)
#define I2C_REG_CURRENT_SPEED 0x30  // Current speed readback (2 bytes, signed)
#define I2C_REG_CURRENT_POS 0x40    // Current position readback (4 bytes, signed)
#define I2C_REG_CURRENT 0x50        // Current sensor (2 bytes, in mA)
#define I2C_REG_VOLTAGE 0x54        // Voltage sensor (2 bytes, in 10mV units/centivolts)
#define I2C_REG_TEMP 0x58           // Temperature sensor (1 byte, in Celsius)
#define I2C_REG_BUTTON 0x60         // Button status (1 byte)
#define I2C_REG_RGB 0x70            // RGB LED control (3 bytes)

// Motor modes
#define I2C_MODE_STOP 0x00
#define I2C_MODE_SPEED 0x01
#define I2C_MODE_POSITION 0x02
#define I2C_MODE_CURRENT 0x03

class UnitRoller485_I2C {
public:
    UnitRoller485_I2C(uint8_t address = 0x64) : _address(address) {}
    
    bool begin(int sda = 32, int scl = 33) {
        Wire.begin(sda, scl);
        Wire.setClock(100000);
        return checkConnection();
    }
    
    bool checkConnection() {
        Wire.beginTransmission(_address);
        return (Wire.endTransmission() == 0);
    }
    
    // Set motor speed in RPM
    bool setSpeed(int16_t speed) {
        Serial.printf("[I2C] Setting mode to SPEED (0x%02X = %d)\n", I2C_REG_MODE, I2C_MODE_SPEED);
        bool modeResult = writeRegister8(I2C_REG_MODE, I2C_MODE_SPEED);
        Serial.printf("[I2C] Mode write result: %s\n", modeResult ? "SUCCESS" : "FAILED");
        delay(5);
        
        Serial.printf("[I2C] Writing speed value %d to register 0x%02X\n", speed, I2C_REG_SPEED);
        bool speedResult = writeRegister16(I2C_REG_SPEED, speed);
        Serial.printf("[I2C] Speed write result: %s\n", speedResult ? "SUCCESS" : "FAILED");
        
        // Read back to verify
        delay(10);
        int16_t readbackSpeed = getSpeed();
        Serial.printf("[I2C] Readback speed: %d RPM\n", readbackSpeed);
        
        return speedResult;
    }
    
    // Set motor position
    bool setPosition(int32_t position, int16_t speed = 500) {
        setSpeed(speed);
        delay(5);
        writeRegister8(I2C_REG_MODE, I2C_MODE_POSITION);
        delay(5);
        return writeRegister32(I2C_REG_POSITION, position);
    }
    
    // Stop motor
    bool stop() {
        return writeRegister8(I2C_REG_MODE, I2C_MODE_STOP);
    }
    
    // Get current speed
    int16_t getSpeed() {
        return readRegister16(I2C_REG_CURRENT_SPEED);
    }
    
    // Get current position
    int32_t getPosition() {
        return readRegister32(I2C_REG_CURRENT_POS);
    }
    
    // Get current in mA
    uint16_t getCurrent() {
        return readRegister16(I2C_REG_CURRENT);
    }
    
    // Get voltage in 10mV units (centivolts) - divide by 100 to get Volts
    uint16_t getVoltage() {
        return readRegister16(I2C_REG_VOLTAGE);
    }
    
    // Get temperature in Celsius
    uint8_t getTemperature() {
        return readRegister8(I2C_REG_TEMP);
    }
    
    // Get button status
    bool getButton() {
        return readRegister8(I2C_REG_BUTTON) != 0;
    }
    
    // Set RGB LED (R, G, B values 0-255)
    bool setRGB(uint8_t r, uint8_t g, uint8_t b) {
        Wire.beginTransmission(_address);
        Wire.write(I2C_REG_RGB);
        Wire.write(r);
        Wire.write(g);
        Wire.write(b);
        return (Wire.endTransmission() == 0);
    }

private:
    uint8_t _address;
    
    bool writeRegister8(uint8_t reg, uint8_t value) {
        Wire.beginTransmission(_address);
        Wire.write(reg);
        Wire.write(value);
        return (Wire.endTransmission() == 0);
    }
    
    bool writeRegister16(uint8_t reg, int16_t value) {
        Wire.beginTransmission(_address);
        Wire.write(reg);
        Wire.write((uint8_t)(value & 0xFF));
        Wire.write((uint8_t)((value >> 8) & 0xFF));
        return (Wire.endTransmission() == 0);
    }
    
    bool writeRegister32(uint8_t reg, int32_t value) {
        Wire.beginTransmission(_address);
        Wire.write(reg);
        Wire.write((uint8_t)(value & 0xFF));
        Wire.write((uint8_t)((value >> 8) & 0xFF));
        Wire.write((uint8_t)((value >> 16) & 0xFF));
        Wire.write((uint8_t)((value >> 24) & 0xFF));
        return (Wire.endTransmission() == 0);
    }
    
    uint8_t readRegister8(uint8_t reg) {
        Wire.beginTransmission(_address);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(_address, (uint8_t)1);
        if (Wire.available()) {
            return Wire.read();
        }
        return 0;
    }
    
    int16_t readRegister16(uint8_t reg) {
        Wire.beginTransmission(_address);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(_address, (uint8_t)2);
        if (Wire.available() >= 2) {
            uint8_t low = Wire.read();
            uint8_t high = Wire.read();
            return (int16_t)((high << 8) | low);
        }
        return 0;
    }
    
    int32_t readRegister32(uint8_t reg) {
        Wire.beginTransmission(_address);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(_address, (uint8_t)4);
        if (Wire.available() >= 4) {
            uint8_t b0 = Wire.read();
            uint8_t b1 = Wire.read();
            uint8_t b2 = Wire.read();
            uint8_t b3 = Wire.read();
            return (int32_t)((b3 << 24) | (b2 << 16) | (b1 << 8) | b0);
        }
        return 0;
    }
};

#endif
