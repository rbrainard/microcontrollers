# M5Stack DMX 512 Controller

This project provides a PlatformIO-based program to control a 32 Channel 96A RGBW DMX 512 LED Decoder Controller using an M5Stack with the M5 DMX 512 module.

## Hardware Requirements

- M5Stack Core (ESP32-based) - BASIC, GRAY, GO, or FIRE
- M5Stack DMX Base Module (connects to bottom of M5Stack)
- 32 Channel 96A RGBW DMX 512 LED Decoder Controller
- DMX cables and terminators
- LED strips or fixtures connected to the decoder
- DC 9-24V power supply for DMX Base module

## Wiring

### M5Stack DMX Base Module
The M5Stack DMX Base module connects directly to the bottom of your M5Stack Core. The module provides:
- XLR-5 and XLR-3 male and female connectors for DMX devices
- Isolated power supply (DC 9-24V input)
- High-speed optocoupler isolation for DMX signals
- Built-in RS-485 transceivers

**Pin Configuration (automatically set in code):**
- **M5Stack BASIC/GRAY/GO/FIRE**: TX=13, RX=35, EN=12
- **M5Stack Core2/Tough**: TX=19, RX=35, EN=27  
- **M5Stack CoreS3**: TX=7, RX=10, EN=6

### DMX Base to LED Controller
1. Connect DMX OUT from M5Stack DMX Base to DMX IN on the LED controller
2. Use proper DMX cables (120Ω twisted pair)
3. Add 120Ω terminator at the end of the DMX chain if this is the last device
4. Power the DMX Base with DC 9-24V (separate from M5Stack USB power)

## Channel Configuration

The 32-channel controller is organized as 8 groups of RGBW channels:

- **Group 1**: Channels 1-4 (R, G, B, W)
- **Group 2**: Channels 5-8 (R, G, B, W)
- **Group 3**: Channels 9-12 (R, G, B, W)
- **Group 4**: Channels 13-16 (R, G, B, W)
- **Group 5**: Channels 17-20 (R, G, B, W)
- **Group 6**: Channels 21-24 (R, G, B, W)
- **Group 7**: Channels 25-28 (R, G, B, W)
- **Group 8**: Channels 29-32 (R, G, B, W)

## Controls

### M5Stack Buttons
- **Button A (Left)**:
  - Short press: Increase channel value by 32 (cycles 0→32→64→96→128→160→192→224→255→0)
  - Long press (1+ seconds): Run color cycle effect on current group

- **Button B (Middle)**:
  - Short press: Switch between channels (R→G→B→W→R...)

- **Button C (Right)**:
  - Short press: Switch between groups (1→2→3→4→5→6→7→8→1...)
  - Long press (1+ seconds): Turn all channels OFF

### Display Information
The M5Stack display shows:
- Current group (1-8)
- Current channel (R, G, B, or W)
- Current channel value (0-255)
- DMX channel number
- DMX transmission status

## Features

### Manual Control
- Individual channel control for precise lighting adjustment
- Group-based organization for easy fixture management
- Real-time value display and feedback

### Built-in Effects
- **Color Cycle**: Cycles through Red, Green, Blue, White, All Colors, and Off
- **Emergency Off**: Quickly turn off all channels

### DMX Communication
- Standard DMX 512 protocol
- Configurable starting channel (default: Channel 1)
- Real-time transmission updates
- Serial debugging output

## Installation

1. Install PlatformIO in VS Code or use PlatformIO CLI
2. Clone or download this project
3. Open the project folder in PlatformIO
4. Connect your M5Stack via USB
5. Build and upload:
   ```bash
   pio run --target upload
   ```

## Configuration

## DMX Controller Setup

Your **32 Channel 96A RGBW DMX 512 LED Decoder Controller** needs to be configured to match the M5Stack settings:

### **DMX Address Configuration:**

1. **Set the starting DMX address on your controller**:
   - Use the 3 buttons under the LCD display on your controller
   - **Button 1** (hold): Enter address setup mode
   - **Buttons 1, 2, 3**: Navigate and set the address number
   - **Default**: Set to address **1** (matches `DMX_START_CHANNEL 1` in code)

2. **Set the control mode**:
   - **Button 2** (hold): Enter control mode setup
   - Select **RGBW mode** for 4-channel groups (Red, Green, Blue, White)
   - This gives you 8 groups × 4 channels = 32 channels total

3. **Set the bit depth**:
   - **Button 3** (hold): Enter bit depth setup
   - Choose **8-bit** (default) or **16-bit** for finer control
   - Update `USE_16_BIT` in the code to match your choice

### **Channel Mapping:**

**8-bit Mode (1 DMX address per channel):**
```
Group 1: Ch 1-4   (R, G, B, W)  → DMX Addresses 1-4
Group 2: Ch 5-8   (R, G, B, W)  → DMX Addresses 5-8
Group 3: Ch 9-12  (R, G, B, W)  → DMX Addresses 9-12
Group 4: Ch 13-16 (R, G, B, W)  → DMX Addresses 13-16
Group 5: Ch 17-20 (R, G, B, W)  → DMX Addresses 17-20
Group 6: Ch 21-24 (R, G, B, W)  → DMX Addresses 21-24
Group 7: Ch 25-28 (R, G, B, W)  → DMX Addresses 25-28
Group 8: Ch 29-32 (R, G, B, W)  → DMX Addresses 29-32
```

**16-bit Mode (2 DMX addresses per channel):**
```
Group 1: Ch 1-4   (R, G, B, W)  → DMX Addresses 1-8   (2 addresses per channel)
Group 2: Ch 5-8   (R, G, B, W)  → DMX Addresses 9-16  (2 addresses per channel)
...and so on (uses 64 DMX addresses total)
```

### **Code Configuration:**

Edit these values in `main.cpp` to match your controller setup:

```cpp
#define DMX_START_CHANNEL 1    // Match your controller's starting address
#define USE_16_BIT false       // Set to true for 16-bit mode, false for 8-bit
```

### Controller Mode
Your LED controller should be set to:
- **Mode**: DMX 512
- **Bit depth**: 8-bit (can be changed to 16-bit if needed)
- **Starting address**: 1 (or match DMX_START_CHANNEL)

## Troubleshooting

### No DMX Output
1. Check DMX Base module power supply (DC 9-24V required)
2. Verify DMX cable connections (XLR-3 or XLR-5)
3. Ensure LED controller is in DMX mode
4. Check DMX addressing matches starting channel
5. Verify DMX termination (120Ω resistor at end of chain)
6. Check that DMX Base switches are in correct position

### M5Stack Issues
1. Verify M5Stack is properly powered
2. Check USB connection for programming
3. Monitor Serial output for debugging info
4. Ensure M5 DMX module is properly connected

### LED Controller Issues
1. Check power supply to LED controller (DC 5-24V)
2. Verify LED strips are properly connected
3. Check controller addressing matches DMX settings
4. Ensure controller is set to correct bit depth (8-bit)

## Serial Debugging

Connect to the serial monitor at 115200 baud to see debugging information:
- DMX channel updates
- Button press events
- Initialization status
- Error messages

## Extending the Code

### Adding New Effects
Add new effect functions similar to `runColorCycle()`:

```cpp
void myCustomEffect() {
    // Your effect code here
    // Use updateDMXChannel() or updateDMXGroup() to control lights
}
```

### Changing Channel Layout
Modify these constants for different channel configurations:
- `GROUPS`: Number of fixture groups
- `CHANNELS_PER_GROUP`: Channels per fixture
- Update channel mapping logic accordingly

### Adding Network Control
Consider adding WiFi capabilities to control the system remotely:
- Web interface
- MQTT integration
- Art-Net support

## License

This project is provided as-is for educational and personal use.