# M5Stack Station 485 with Unit Roller485 Brushless Motor Controller

This project provides a complete solution for controlling M5Stack Unit Roller485 brushless motors using an M5Stack Station 485 with wireless control capabilities through both a built-in web interface and a desktop Python application.

## Features

- **Complete Motor Control**: Velocity, Position, and Current control modes
- **WiFi Connectivity**: Built-in web server with REST API
- **Local Display**: Real-time status display on Station 485 screen
- **Python Desktop UI**: Advanced GUI with real-time monitoring and plotting
- **Safety Features**: Emergency stop, error handling, and position limits
- **Multiple Interfaces**: Web browser, Python GUI, and physical buttons

## Hardware Requirements

### Core Components
- M5Stack Station 485 (ESP32-based industrial controller)
- M5Stack Unit Roller485 (Brushless motor with integrated controller)
- USB-C cable for programming and power
- WiFi network for wireless control
- 9-24V power supply for Station 485 PWR485 input

### Technical Specifications

#### M5Stack Station 485
- **MCU**: ESP32-D0WDQ6-V3 @ 240MHz dual-core
- **Memory**: 520KB SRAM, 16MB Flash
- **Display**: 1.14" IPS LCD (240x135) ST7789V2
- **Connectivity**: Wi-Fi 2.4GHz, RS485 (SP3485)
- **Power**: 9-24V via PWR485 connector, USB-C 5V
- **I/O**: 6x Grove ports, RGB LEDs, 3 buttons

#### Unit Roller485
- **MCU**: STM32G431CBU6 @ 170MHz
- **Motor**: D3504 200KV brushless motor (Ø41mm)
- **Driver**: DRV8311HRRWR with FOC control
- **Encoder**: TLI5012BE1000 magnetic encoder
- **Power**: 6-16V (PWR485) or 5V (Grove)
- **Communication**: RS485, I2C
- **Features**: 0.66" OLED display, RGB LEDs, slip ring option

### Wiring Diagram

```
M5Stack Station 485 Connections:
┌─────────────────────────┐
│    M5Stack Station 485  │
│      (ESP32-based)      │
├─────────────────────────┤
│ PWR485 Interface:       │
│   RS485_A (GPIO17) ─────┼──── Unit Roller485 RS485_A
│   RS485_B (GPIO18) ─────┼──── Unit Roller485 RS485_B  
│   DE (GPIO19)      ─────┼──── Unit Roller485 RS485_DE
│   9-24V Power      ─────┼──── Unit Roller485 Power
│   GND              ─────┼──── Unit Roller485 GND
└─────────────────────────┘
         │
         ▼
┌─────────────────────────┐      ┌─────────────────┐
│   Unit Roller485        │      │   Motor Load    │
│  (STM32 + Motor)        │ ───► │  (Your System)  │
│                         │      │                 │
│ - Velocity Control      │      │ - Robotic Joint │
│ - Position Control      │      │ - Turntable     │
│ - Current Control       │      │ - Actuator      │
│ - Built-in Encoder      │      │ - etc.          │
└─────────────────────────┘      └─────────────────┘

Power Supply Requirements:
- Station 485: 9-24V DC @ 1A (PWR485 connector)
- Unit Roller485: Powered via RS485 connection
- Motor: Integrated with Unit Roller485
- Communication: RS485 @ 115200 baud
```

## Software Setup

### PlatformIO Firmware

1. **Install PlatformIO IDE**
   - Install VS Code
   - Install PlatformIO extension

2. **Open Project**
   ```bash
   cd 485Brushless
   pio run
   ```

3. **Configure WiFi**
   Edit `src/main.cpp` and update:
   ```cpp
   #define WIFI_SSID "YourWiFiSSID"
   #define WIFI_PASSWORD "YourWiFiPassword"
   ```

4. **Upload Firmware**
   ```bash
   pio run --target upload
   ```

### Python UI Setup

1. **Install Python 3.8+**

2. **Install Dependencies**
   ```bash
   cd python_ui
   pip install -r requirements.txt
   ```

3. **Run Application**
   ```bash
   python motor_controller_ui.py
   ```

## Usage Guide

### M5Stack Station 485 Local Interface

The Station 485 provides a local interface with four display modes:

#### Button Controls
- **Button A (Left)**: Previous display mode
- **Button B (Center)**: Action button (context-sensitive)
- **Button C (Right)**: Next display mode
- **Button A + B**: Emergency stop

#### Display Modes

1. **STATUS Mode**
   - Shows real-time motor status
   - Running state, velocity, position, current
   - Voltage, temperature, encoder status
   - Error codes and messages
   - Action: Toggle motor enable/disable

2. **CONTROL Mode**
   - Manual control interface
   - Shows target values for velocity, position, current
   - Action: Execute current control command

3. **CONFIG Mode**
   - Motor configuration display
   - Maximum velocity, acceleration, PID parameters
   - Position limits and encoder settings
   - Action: Reset motor position

4. **NETWORK Mode**
   - WiFi connection status
   - IP address and web server info
   - Connection statistics
   - Action: Calibrate encoder

### Web Interface

Access the built-in web interface by navigating to the Station 485's IP address:

```
http://[Station485_IP_Address]
```

The web interface provides:
- Real-time status display
- Velocity, position, and current control
- Motor start/stop controls
- Emergency stop button
- Error management

### Python Desktop Application

The Python GUI provides advanced features:

#### Connection Setup
1. Enter Station 485 IP address and port (default: 80)
2. Click "Connect"
3. Enable "Auto Update" for real-time monitoring

#### Control Features
- **Velocity Control**: Set target RPM
- **Position Control**: Set target position with velocity
- **Current Control**: Set target current value
- **Safety Controls**: Stop, disable, reset position, calibrate

#### Monitoring Features
- Real-time status text display
- Live plotting of velocity, position, voltage, current, temperature
- Activity log with timestamps
- Historical data tracking

## API Reference

### REST API Endpoints

#### Status Endpoints
- `GET /api/status` - Get current motor status
- `GET /api/config` - Get motor configuration

#### Control Endpoints
- `POST /api/control/velocity` - Set velocity mode
  ```json
  {"velocity": 100.0}
  ```

- `POST /api/control/position` - Set position mode
  ```json
  {"position": 18000, "velocity": 500.0}
  ```

- `POST /api/control/current` - Set current mode
  ```json
  {"current": 300}
  ```

- `POST /api/control/stop` - Stop motor
- `POST /api/control/disable` - Disable motor
- `POST /api/control/reset_position` - Reset position to zero
- `POST /api/control/calibrate` - Calibrate encoder
- `POST /api/control/enable` - Enable motor
  ```json
  {"enable": true}
  ```

#### Response Format
```json
{
  "success": true,
  "message": "Command executed successfully",
  "timestamp": 12345678
}
```

#### Status Response Format
```json
{
  "isRunning": true,
  "mode": "Velocity",
  "direction": "CW",
  "velocity": 100.5,
  "position": 18000,
  "current": 0.25,
  "voltage": 12.1,
  "temperature": 35,
  "encoderReady": true,
  "errorCode": 0,
  "errorMessage": "No Error",
  "timestamp": 12345678
}
```

## Configuration

### Motor Parameters

Default motor configuration for Unit Roller485:

```cpp
MotorConfig config;
config.motorId = MOTOR_ID;
config.maxVelocity = 2000.0;    // Maximum RPM
config.acceleration = 500.0;    // RPM/s
config.maxCurrent = 500.0;      // mA (0.5A continuous)
config.enableLimits = false;
config.minPosition = 0;
config.maxPosition = 36000;     // 360° in encoder counts
// PID parameters...
```

### Communication Settings

RS485 communication parameters:

```cpp
#define RS485_RX_PIN 18         // Station 485 RX pin
#define RS485_TX_PIN 17         // Station 485 TX pin  
#define RS485_DE_PIN 19         // Direction Enable pin
```

Baud rate: 115200 bps (Unit Roller485 standard)

### Position Encoding

Unit Roller485 encoder specifications:
- **Resolution**: 36000 counts per 360°
- **Conversion**: 1° = 100 encoder counts
- **Range**: 0-36000 (one full rotation)
- **Type**: Absolute magnetic encoder (TLI5012BE1000)

## Troubleshooting

### Connection Issues

**Motor not responding:**
1. Check RS485 wiring (A, B terminals on PWR485 connector)
2. Verify power supply to Station 485 (9-24V)
3. Check baud rate settings (115200)
4. Ensure motor ID matches configuration

**WiFi connection failed:**
1. Check SSID and password
2. Ensure WiFi network is available
3. Check signal strength
4. Try Access Point mode

**Python UI cannot connect:**
1. Verify Station 485 IP address
2. Check firewall settings
3. Ensure web server is running
4. Try ping test to Station 485

### Motor Control Issues

**Motor not moving:**
1. Check Unit Roller485 power supply (6-16V)
2. Verify motor is enabled
3. Check for error codes on OLED display
4. Ensure velocity/current parameters are within limits

**Erratic movement:**
1. Check encoder calibration
2. Verify PID parameters
3. Check for mechanical binding
4. Review load specifications

**Communication errors:**
1. Check RS485 cable integrity
2. Verify baud rate (115200)
3. Check for noise interference
4. Review motor ID settings

### Display Issues

**Station 485 display not updating:**
1. Check for firmware errors in serial monitor
2. Verify button connections
3. Restart Station 485
4. Re-upload firmware

**Unit Roller485 OLED showing errors:**
1. Check power supply voltage (6-16V)
2. Verify encoder connections
3. Check for overtemperature
4. Review current limits

## Error Codes

| Code | Description | Solution |
|------|-------------|----------|
| 0x00 | No Error | Normal operation |
| 0x01 | Overvoltage | Check power supply (max 16V) |
| 0x02 | Undervoltage | Check power supply (min 6V) |
| 0x03 | Overcurrent | Check motor load and current limits |
| 0x04 | Overtemperature | Check ventilation and load |
| 0x05 | Position Limit | Check position limits and commands |
| 0x06 | Communication Error | Check RS485 connections |
| 0x07 | Motor Stall | Check mechanical load |
| 0x08 | Encoder Error | Check encoder calibration |
| 0x09 | Calibration Required | Run encoder calibration |
| 0x0A | Motor Disabled | Enable motor first |

## Safety Considerations

1. **Power Limits**: Never exceed 16V input to Unit Roller485
2. **Current Limits**: Continuous operation limited to 0.5A
3. **Temperature**: Monitor operating temperature (0-40°C)
4. **Emergency Stop**: Always ensure emergency stop is accessible
5. **Mechanical Safety**: Ensure proper guards and safety devices
6. **Testing**: Test all safety functions before operation

## Performance Specifications

### Unit Roller485 Performance
- **No Load**: 16V/78mA
- **50g Load**: 2100 RPM @ 16V/225mA
- **200g Load**: 1400 RPM @ 16V/601mA  
- **500g Load (Max)**: 560 RPM @ 16V/918mA
- **Torque**: 0.065 N⋅m @ 16V, 0.021 N⋅m @ 5V
- **Noise Level**: 48dB

## License

This project is provided as-is for educational and development purposes. Please ensure compliance with local safety regulations when implementing motor control systems.

## Support

For technical support:
1. Check this documentation
2. Review error codes and troubleshooting
3. Consult M5Stack documentation
4. Test with minimal configurations first

---

**Version**: 2.0  
**Date**: October 2025  
**Hardware**: M5Stack Station 485 + Unit Roller485  
**Author**: Motor Controller Development Team

## Software Setup

### PlatformIO Firmware

1. **Install PlatformIO IDE**
   - Install VS Code
   - Install PlatformIO extension

2. **Open Project**
   ```bash
   cd 485Brushless
   pio run
   ```

3. **Configure WiFi**
   Edit `src/main.cpp` and update:
   ```cpp
   #define WIFI_SSID "YourWiFiSSID"
   #define WIFI_PASSWORD "YourWiFiPassword"
   ```

4. **Upload Firmware**
   ```bash
   pio run --target upload
   ```

### Python UI Setup

1. **Install Python 3.8+**

2. **Install Dependencies**
   ```bash
   cd python_ui
   pip install -r requirements.txt
   ```

3. **Run Application**
   ```bash
   python motor_controller_ui.py
   ```

## Usage Guide

### M5Station Local Interface

The M5Station provides a local interface with four display modes:

#### Button Controls
- **Button A (Left)**: Previous display mode
- **Button B (Center)**: Action button (context-sensitive)
- **Button C (Right)**: Next display mode
- **Button A + B**: Emergency stop

#### Display Modes

1. **STATUS Mode**
   - Shows real-time motor status
   - Running state, speed, position, torque
   - Voltage, current, temperature
   - Error codes and messages
   - Action: Toggle motor enable/disable

2. **CONTROL Mode**
   - Manual control interface
   - Shows target values for speed, position, torque
   - Action: Execute current control command

3. **CONFIG Mode**
   - Motor configuration display
   - Maximum speed, acceleration/deceleration
   - Position limits
   - Action: Reset motor position

4. **NETWORK Mode**
   - WiFi connection status
   - IP address and web server info
   - Connection statistics
   - Action: Clear motor errors

### Web Interface

Access the built-in web interface by navigating to the M5Station's IP address in a web browser:

```
http://[M5Station_IP_Address]
```

The web interface provides:
- Real-time status display
- Speed, position, and torque control
- Motor start/stop controls
- Emergency stop button
- Error management

### Python Desktop Application

The Python GUI provides advanced features:

#### Connection Setup
1. Enter M5Station IP address and port (default: 80)
2. Click "Connect"
3. Enable "Auto Update" for real-time monitoring

#### Control Features
- **Speed Control**: Set target RPM
- **Position Control**: Set target position with speed
- **Torque Control**: Set target torque value
- **Safety Controls**: Stop, emergency stop, reset position, clear errors

#### Monitoring Features
- Real-time status text display
- Live plotting of speed, position, voltage, current, temperature
- Activity log with timestamps
- Historical data tracking

## API Reference

### REST API Endpoints

#### Status Endpoints
- `GET /api/status` - Get current motor status
- `GET /api/config` - Get motor configuration

#### Control Endpoints
- `POST /api/control/speed` - Set speed mode
  ```json
  {"speed": 1000}
  ```

- `POST /api/control/position` - Set position mode
  ```json
  {"position": 5000, "speed": 500}
  ```

- `POST /api/control/torque` - Set torque mode
  ```json
  {"torque": 100}
  ```

- `POST /api/control/stop` - Stop motor
- `POST /api/control/emergency_stop` - Emergency stop
- `POST /api/control/reset_position` - Reset position to zero
- `POST /api/control/clear_errors` - Clear error flags
- `POST /api/control/enable` - Enable/disable motor
  ```json
  {"enable": true}
  ```

#### Response Format
```json
{
  "success": true,
  "message": "Command executed successfully",
  "timestamp": 12345678
}
```

#### Status Response Format
```json
{
  "isRunning": true,
  "mode": "Speed",
  "direction": "CW",
  "speed": 1000,
  "position": 5000,
  "torque": 50,
  "voltage": 24.1,
  "current": 2.5,
  "temperature": 45,
  "errorCode": 0,
  "errorMessage": "No Error",
  "timestamp": 12345678
}
```

## Configuration

### Motor Parameters

Default motor configuration can be modified in `src/main.cpp`:

```cpp
MotorConfig config;
config.motorId = MOTOR_ID;
config.maxSpeed = 3000;        // Maximum RPM
config.acceleration = 1000;    // Acceleration rate
config.deceleration = 1000;    // Deceleration rate
config.enableLimits = false;   // Enable position limits
config.minPosition = -100000;  // Minimum position
config.maxPosition = 100000;   // Maximum position
```

### Communication Settings

RS485 communication parameters in `src/main.cpp`:

```cpp
#define RS485_RX_PIN 16      // RX pin
#define RS485_TX_PIN 17      // TX pin  
#define RS485_DE_PIN 5       // Direction Enable pin
```

Baud rate: 9600 bps (configurable in motor controller initialization)

### Network Settings

WiFi configuration:

```cpp
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"
#define WEB_SERVER_PORT 80
```

For Access Point mode (if WiFi connection fails):
- SSID: "M5-Motor-Controller"
- Password: "12345678"

## Troubleshooting

### Connection Issues

**Motor not responding:**
1. Check RS485 wiring (A, B terminals)
2. Verify power supply to PWR485 controller
3. Check baud rate settings
4. Ensure motor ID matches configuration

**WiFi connection failed:**
1. Check SSID and password
2. Ensure WiFi network is available
3. Check signal strength
4. Try Access Point mode

**Python UI cannot connect:**
1. Verify M5Station IP address
2. Check firewall settings
3. Ensure web server is running on M5Station
4. Try ping test to M5Station

### Motor Control Issues

**Motor not moving:**
1. Check motor power supply
2. Verify motor is enabled
3. Check for error codes
4. Ensure motor parameters are within limits

**Erratic movement:**
1. Check encoder connections
2. Verify acceleration/deceleration settings
3. Check for mechanical binding
4. Review torque limits

**Communication errors:**
1. Check RS485 termination
2. Verify cable integrity
3. Check noise interference
4. Review baud rate settings

### Display Issues

**M5Station display not updating:**
1. Check for firmware errors in serial monitor
2. Verify button connections
3. Restart M5Station
4. Re-upload firmware

**Python plots not showing:**
1. Ensure matplotlib is installed
2. Check auto-update is enabled
3. Verify connection status
4. Review error logs

## Error Codes

| Code | Description | Solution |
|------|-------------|----------|
| 0x00 | No Error | Normal operation |
| 0x01 | Overvoltage | Check power supply voltage |
| 0x02 | Undervoltage | Check power supply and connections |
| 0x03 | Overcurrent | Check motor load and wiring |
| 0x04 | Overtemperature | Check ventilation and load |
| 0x05 | Position Limit | Check position limits and commands |
| 0x06 | Communication Error | Check RS485 connections |
| 0x07 | Motor Stall | Check mechanical load |
| 0x08 | Encoder Error | Check encoder connections |

## Safety Considerations

1. **Emergency Stop**: Always ensure emergency stop is accessible
2. **Power Isolation**: Use proper isolation for motor power supply
3. **Mechanical Safety**: Ensure proper guards and safety devices
4. **Testing**: Test all safety functions before operation
5. **Monitoring**: Continuously monitor temperature and current
6. **Limits**: Set appropriate position and speed limits

## Development Notes

### Adding New Features

1. **Motor Controller**: Modify `M5PWR485MotorController.h/.cpp`
2. **Web API**: Add endpoints in `MotorWebServer.cpp`
3. **Display**: Update display modes in `main.cpp`
4. **Python UI**: Extend GUI in `motor_controller_ui.py`

### Protocol Extensions

The RS485 protocol can be extended by:
1. Adding new command codes
2. Extending data structures
3. Implementing additional CRC checks
4. Adding encryption for security

## License

This project is provided as-is for educational and development purposes. Please ensure compliance with local safety regulations when implementing motor control systems.

## Support

For technical support:
1. Check this documentation
2. Review error codes and troubleshooting
3. Consult M5Stack and motor controller documentation
4. Test with minimal configurations first

---

**Version**: 1.0  
**Date**: October 2025  
**Author**: Motor Controller Development Team