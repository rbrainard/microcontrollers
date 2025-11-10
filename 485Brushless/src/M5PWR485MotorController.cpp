#include "M5PWR485MotorController.h"

// Unit Roller485 Communication Protocol Constants
#define FRAME_HEADER_SIZE   3   // Motor ID + Function + Register
#define CRC_SIZE           2
#define MAX_RESPONSE_TIME  100  // ms

// Function codes for Modbus-like protocol
#define FUNC_READ_HOLDING_REGISTER  0x03
#define FUNC_WRITE_SINGLE_REGISTER  0x06
#define FUNC_WRITE_MULTIPLE_REGISTERS 0x10

UnitRoller485Controller::UnitRoller485Controller(uint8_t motorId) 
    : _motorId(motorId), _serial(nullptr), _baudRate(115200), 
      _rxPin(18), _txPin(17), _dePin(19) {
    // Initialize status
    _status = {false, MODE_DISABLE, DIR_CW, 0.0, 0, 0.0, 0.0, 0, 0, false};
    
    // Initialize config with defaults for Unit Roller485
    _config = {motorId, 2000.0, 500.0, 500.0, false, 0, 36000, 100, 50, 10, 200, 100, 20};
}

UnitRoller485Controller::~UnitRoller485Controller() {
    if (_serial) {
        _serial->end();
    }
}

bool UnitRoller485Controller::begin(HardwareSerial* serial, uint32_t baudRate, 
                                    uint8_t rxPin, uint8_t txPin, uint8_t dePin) {
    _serial = serial;
    _baudRate = baudRate;
    _rxPin = rxPin;
    _txPin = txPin;
    _dePin = dePin;
    
    // Configure DE pin for RS485 direction control
    pinMode(_dePin, OUTPUT);
    enableReceive();
    
    // Initialize serial communication
    _serial->begin(_baudRate, SERIAL_8N1, _rxPin, _txPin);
    delay(100);
    
    // Test connection
    return isConnected();
}

bool UnitRoller485Controller::isConnected() {
    return updateStatus();
}

void UnitRoller485Controller::enableTransmit() {
    digitalWrite(_dePin, HIGH);
    delayMicroseconds(50);
}

void UnitRoller485Controller::enableReceive() {
    digitalWrite(_dePin, LOW);
    delayMicroseconds(50);
}

uint16_t UnitRoller485Controller::calculateCRC(uint8_t* data, uint8_t length) {
    uint16_t crc = 0xFFFF;
    
    for (uint8_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

bool UnitRoller485Controller::writeRegister(uint8_t reg, uint32_t value) {
    uint8_t frame[8];
    
    // Build Modbus RTU frame for single register write
    frame[0] = _motorId;
    frame[1] = FUNC_WRITE_SINGLE_REGISTER;
    frame[2] = reg;
    frame[3] = 0x00;  // Register high byte (assuming 16-bit registers)
    frame[4] = (value >> 8) & 0xFF;  // Data high byte
    frame[5] = value & 0xFF;         // Data low byte
    
    // Calculate CRC
    uint16_t crc = calculateCRC(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = (crc >> 8) & 0xFF;
    
    // Send frame
    enableTransmit();
    _serial->write(frame, 8);
    _serial->flush();
    enableReceive();
    
    // Wait for response
    uint32_t startTime = millis();
    uint8_t response[8];
    uint8_t responseIndex = 0;
    
    while (millis() - startTime < MAX_RESPONSE_TIME && responseIndex < 8) {
        if (_serial->available()) {
            response[responseIndex++] = _serial->read();
        }
    }
    
    // Verify response (should echo the command for successful write)
    if (responseIndex >= 8) {
        uint16_t receivedCRC = response[6] | (response[7] << 8);
        uint16_t calculatedCRC = calculateCRC(response, 6);
        return (receivedCRC == calculatedCRC) && (response[0] == _motorId);
    }
    
    return false;
}

bool UnitRoller485Controller::readRegister(uint8_t reg, uint32_t& value) {
    uint8_t frame[8];
    
    // Build Modbus RTU frame for register read
    frame[0] = _motorId;
    frame[1] = FUNC_READ_HOLDING_REGISTER;
    frame[2] = 0x00;  // Start register high byte
    frame[3] = reg;   // Start register low byte
    frame[4] = 0x00;  // Number of registers high byte
    frame[5] = 0x01;  // Number of registers low byte (read 1 register)
    
    // Calculate CRC
    uint16_t crc = calculateCRC(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = (crc >> 8) & 0xFF;
    
    // Send frame
    enableTransmit();
    _serial->write(frame, 8);
    _serial->flush();
    enableReceive();
    
    // Wait for response
    uint32_t startTime = millis();
    uint8_t response[7];  // ID + FUNC + COUNT + DATA(2) + CRC(2)
    uint8_t responseIndex = 0;
    
    while (millis() - startTime < MAX_RESPONSE_TIME && responseIndex < 7) {
        if (_serial->available()) {
            response[responseIndex++] = _serial->read();
        }
    }
    
    // Verify and parse response
    if (responseIndex >= 7 && response[0] == _motorId && response[1] == FUNC_READ_HOLDING_REGISTER) {
        uint16_t receivedCRC = response[5] | (response[6] << 8);
        uint16_t calculatedCRC = calculateCRC(response, 5);
        
        if (receivedCRC == calculatedCRC) {
            value = (response[3] << 8) | response[4];
            return true;
        }
    }
    
    return false;
}

bool UnitRoller485Controller::setVelocityMode(float velocity_rpm) {
    // Set mode to velocity
    if (!writeRegister(REG_MOTOR_MODE, MODE_VELOCITY)) {
        return false;
    }
    
    // Set velocity (convert RPM to internal units, typically RPM * 10)
    uint32_t velocityValue = (uint32_t)(velocity_rpm * 10.0);
    if (writeRegister(REG_MOTOR_SPEED, velocityValue)) {
        _status.currentMode = MODE_VELOCITY;
        _status.velocity = velocity_rpm;
        _status.isRunning = true;
        return true;
    }
    
    return false;
}

bool UnitRoller485Controller::setPositionMode(int32_t position, float velocity_rpm) {
    // Set mode to position
    if (!writeRegister(REG_MOTOR_MODE, MODE_POSITION)) {
        return false;
    }
    
    // Set position target
    if (!writeRegister(REG_MOTOR_POSITION, position)) {
        return false;
    }
    
    // Set velocity for position move
    uint32_t velocityValue = (uint32_t)(velocity_rpm * 10.0);
    if (writeRegister(REG_MOTOR_SPEED, velocityValue)) {
        _status.currentMode = MODE_POSITION;
        _status.isRunning = true;
        return true;
    }
    
    return false;
}

bool UnitRoller485Controller::setCurrentMode(float current_ma) {
    // Set mode to current
    if (!writeRegister(REG_MOTOR_MODE, MODE_CURRENT)) {
        return false;
    }
    
    // Set current target (in mA)
    uint32_t currentValue = (uint32_t)current_ma;
    if (writeRegister(REG_MOTOR_CURRENT, currentValue)) {
        _status.currentMode = MODE_CURRENT;
        _status.current = current_ma / 1000.0;  // Convert to A for display
        _status.isRunning = true;
        return true;
    }
    
    return false;
}

bool UnitRoller485Controller::disableMotor() {
    if (writeRegister(REG_MOTOR_MODE, MODE_DISABLE)) {
        _status.currentMode = MODE_DISABLE;
        _status.isRunning = false;
        _status.velocity = 0.0;
        return true;
    }
    return false;
}

bool UnitRoller485Controller::enableMotor() {
    // For Unit Roller485, enabling means setting a control mode
    // Default to velocity mode with 0 RPM
    return setVelocityMode(0.0);
}

bool UnitRoller485Controller::updateStatus() {
    uint32_t value;
    
    // Read motor status register
    if (!readRegister(REG_MOTOR_STATUS, value)) {
        return false;
    }
    
    // Parse status (simplified - actual format depends on Unit Roller485 firmware)
    _status.isRunning = (value & 0x01) != 0;
    _status.currentMode = (MotorMode)((value >> 1) & 0x03);
    _status.encoderReady = (value & 0x08) != 0;
    _status.errorCode = (value >> 8) & 0xFF;
    
    // Read current position
    if (readRegister(REG_ENCODER_POSITION, value)) {
        _status.position = (int32_t)value;
    }
    
    // Read actual velocity
    if (readRegister(REG_MOTOR_SPEED, value)) {
        _status.velocity = (float)value / 10.0;  // Convert from internal units
    }
    
    // Read motor voltage
    if (readRegister(REG_MOTOR_VOLTAGE, value)) {
        _status.voltage = (float)value / 1000.0;  // Convert from mV to V
    }
    
    // Read motor current
    if (readRegister(REG_MOTOR_CURRENT_ACTUAL, value)) {
        _status.current = (float)value / 1000.0;  // Convert from mA to A
    }
    
    // Read temperature
    if (readRegister(REG_MOTOR_TEMPERATURE, value)) {
        _status.temperature = (int)value;
    }
    
    return true;
}

bool UnitRoller485Controller::resetPosition() {
    // Reset encoder position to 0
    return writeRegister(REG_ENCODER_POSITION, 0);
}

bool UnitRoller485Controller::calibrateEncoder() {
    // Trigger encoder calibration (implementation depends on specific firmware)
    // This is a placeholder - actual implementation would depend on Unit Roller485 protocol
    return writeRegister(0xFF, 0x01);  // Special calibration command
}

int32_t UnitRoller485Controller::degreesToPosition(float degrees) {
    // Unit Roller485: 36000 encoder counts = 360 degrees
    return (int32_t)(degrees * 100.0);
}

float UnitRoller485Controller::positionToDegrees(int32_t position) {
    // Unit Roller485: 36000 encoder counts = 360 degrees
    return (float)position / 100.0;
}

bool UnitRoller485Controller::setMotorConfig(const MotorConfig& config) {
    _config = config;
    
    bool success = true;
    success &= writeRegister(REG_MAX_VELOCITY, (uint32_t)(config.maxVelocity * 10.0));
    success &= writeRegister(REG_MAX_CURRENT, (uint32_t)config.maxCurrent);
    success &= writeRegister(REG_ACCELERATION, (uint32_t)(config.acceleration * 10.0));
    success &= writeRegister(REG_PID_VELOCITY_KP, config.kp_velocity);
    success &= writeRegister(REG_PID_VELOCITY_KI, config.ki_velocity);
    success &= writeRegister(REG_PID_VELOCITY_KD, config.kd_velocity);
    success &= writeRegister(REG_PID_POSITION_KP, config.kp_position);
    success &= writeRegister(REG_PID_POSITION_KI, config.ki_position);
    success &= writeRegister(REG_PID_POSITION_KD, config.kd_position);
    
    return success;
}

MotorConfig UnitRoller485Controller::getMotorConfig() {
    return _config;
}

MotorStatus UnitRoller485Controller::getStatus() {
    return _status;
}

String UnitRoller485Controller::getStatusString() {
    String status = "Unit Roller485 ID: " + String(_motorId) + "\n";
    status += "Running: " + String(_status.isRunning ? "Yes" : "No") + "\n";
    status += "Mode: ";
    
    switch (_status.currentMode) {
        case MODE_VELOCITY: status += "Velocity"; break;
        case MODE_POSITION: status += "Position"; break;
        case MODE_CURRENT: status += "Current"; break;
        case MODE_DISABLE: status += "Disabled"; break;
        default: status += "Unknown"; break;
    }
    
    status += "\nVelocity: " + String(_status.velocity, 1) + " RPM\n";
    status += "Position: " + String(_status.position) + " (" + String(positionToDegrees(_status.position), 1) + "°)\n";
    status += "Current: " + String(_status.current, 3) + " A\n";
    status += "Voltage: " + String(_status.voltage, 1) + " V\n";
    status += "Temperature: " + String(_status.temperature) + " °C\n";
    status += "Encoder: " + String(_status.encoderReady ? "Ready" : "Not Ready") + "\n";
    
    if (_status.errorCode != 0) {
        status += "Error: " + getErrorString(_status.errorCode) + "\n";
    }
    
    return status;
}

String UnitRoller485Controller::getErrorString(uint8_t errorCode) {
    switch (errorCode) {
        case 0x00: return "No Error";
        case 0x01: return "Overvoltage";
        case 0x02: return "Undervoltage";
        case 0x03: return "Overcurrent";
        case 0x04: return "Overtemperature";
        case 0x05: return "Position Limit";
        case 0x06: return "Communication Error";
        case 0x07: return "Motor Stall";
        case 0x08: return "Encoder Error";
        case 0x09: return "Calibration Required";
        case 0x0A: return "Motor Disabled";
        default: return "Unknown Error (" + String(errorCode, HEX) + ")";
    }
}