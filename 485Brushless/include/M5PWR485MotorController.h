#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

// Unit Roller485 Motor Control Commands and Registers
enum MotorMode {
    MODE_DISABLE = 0x00,   // Motor disabled
    MODE_VELOCITY = 0x01,  // Velocity control mode (RPM)
    MODE_POSITION = 0x02,  // Position control mode 
    MODE_CURRENT = 0x03    // Current control mode
};

enum MotorDirection {
    DIR_CW = 0x01,         // Clockwise
    DIR_CCW = 0x02         // Counter-clockwise
};

struct MotorStatus {
    bool isRunning;
    MotorMode currentMode;
    MotorDirection direction;
    float velocity;        // RPM (actual)
    int32_t position;      // Encoder position (0-36000 = 0-360°)
    float current;         // Motor current (A)
    float voltage;         // Motor voltage (V)
    int temperature;       // Motor temperature (°C)
    uint8_t errorCode;     // Error status
    bool encoderReady;     // Encoder status
};

struct MotorConfig {
    uint8_t motorId;       // Motor ID (1-247)
    float maxVelocity;     // Maximum RPM
    float acceleration;    // Acceleration rate (RPM/s)
    float maxCurrent;      // Maximum current (A)
    bool enableLimits;     // Enable position limits
    int32_t minPosition;   // Minimum position (encoder counts)
    int32_t maxPosition;   // Maximum position (encoder counts)
    uint16_t kp_velocity;  // Velocity PID Kp
    uint16_t ki_velocity;  // Velocity PID Ki
    uint16_t kd_velocity;  // Velocity PID Kd
    uint16_t kp_position;  // Position PID Kp
    uint16_t ki_position;  // Position PID Ki
    uint16_t kd_position;  // Position PID Kd
};

// Unit Roller485 Register Map
#define REG_MOTOR_MODE          0x00
#define REG_MOTOR_SPEED         0x01  // Velocity setpoint (RPM * 10)
#define REG_MOTOR_POSITION      0x02  // Position setpoint (encoder counts)
#define REG_MOTOR_CURRENT       0x03  // Current setpoint (mA)
#define REG_MOTOR_STATUS        0x04  // Motor status
#define REG_ENCODER_POSITION    0x05  // Current encoder position
#define REG_MOTOR_VOLTAGE       0x06  // Motor voltage (mV)
#define REG_MOTOR_CURRENT_ACTUAL 0x07 // Actual motor current (mA)
#define REG_MOTOR_TEMPERATURE   0x08  // Motor temperature
#define REG_PID_VELOCITY_KP     0x10  // Velocity PID parameters
#define REG_PID_VELOCITY_KI     0x11
#define REG_PID_VELOCITY_KD     0x12
#define REG_PID_POSITION_KP     0x13  // Position PID parameters
#define REG_PID_POSITION_KI     0x14
#define REG_PID_POSITION_KD     0x15
#define REG_MAX_VELOCITY        0x20  // Maximum velocity (RPM * 10)
#define REG_MAX_CURRENT         0x21  // Maximum current (mA)
#define REG_ACCELERATION        0x22  // Acceleration (RPM/s * 10)

class UnitRoller485Controller {
private:
    HardwareSerial* _serial;
    uint8_t _motorId;
    MotorStatus _status;
    MotorConfig _config;
    
    // Communication parameters
    uint32_t _baudRate;
    uint8_t _rxPin;
    uint8_t _txPin;
    uint8_t _dePin;  // Direction enable pin for RS485
    
    // Internal methods
    bool sendCommand(uint8_t reg, uint32_t value);
    bool readRegister(uint8_t reg, uint32_t& value);
    bool writeRegister(uint8_t reg, uint32_t value);
    uint16_t calculateCRC(uint8_t* data, uint8_t length);
    void enableTransmit();
    void enableReceive();
    
public:
    UnitRoller485Controller(uint8_t motorId = 1);
    ~UnitRoller485Controller();
    
    // Initialization
    bool begin(HardwareSerial* serial, uint32_t baudRate = 115200, 
               uint8_t rxPin = 18, uint8_t txPin = 17, uint8_t dePin = 19);
    bool isConnected();
    
    // Motor control methods
    bool setVelocityMode(float velocity_rpm);  // Velocity in RPM
    bool setPositionMode(int32_t position, float velocity_rpm = 100.0);
    bool setCurrentMode(float current_ma);
    bool disableMotor();
    bool enableMotor();
    
    // Configuration methods
    bool setMotorConfig(const MotorConfig& config);
    MotorConfig getMotorConfig();
    bool setMaxVelocity(float maxVelocity);
    bool setAcceleration(float acceleration);
    bool setPositionLimits(int32_t minPos, int32_t maxPos);
    bool setPIDParameters(uint16_t kp_vel, uint16_t ki_vel, uint16_t kd_vel,
                         uint16_t kp_pos, uint16_t ki_pos, uint16_t kd_pos);
    
    // Status and monitoring
    bool updateStatus();
    MotorStatus getStatus();
    bool resetPosition();
    bool calibrateEncoder();
    
    // Position conversion utilities (36000 encoder counts = 360°)
    int32_t degreesToPosition(float degrees);
    float positionToDegrees(int32_t position);
    
    // Utility methods
    String getStatusString();
    String getErrorString(uint8_t errorCode);
};