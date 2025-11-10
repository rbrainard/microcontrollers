# M5Stack DMX Controller - ESPHome Version

## Overview
This ESPHome configuration turns your M5Stack with DMX Base module into a Home Assistant-controlled DMX 512 LED controller for your 32-channel RGBW LED strips.

## Features
- **8 RGBW Light Groups**: Control 8 groups of 4-channel RGBW lights (32 channels total)
- **Home Assistant Integration**: Full control through Home Assistant UI
- **Local Controls**: M5Stack buttons for local operation
- **Display Feedback**: LCD shows connection status and light values
- **Built-in Effects**: Random, strobe, and flicker effects for each group
- **OTA Updates**: Update firmware wirelessly
- **WiFi Management**: Automatic connection with fallback hotspot

## Hardware Requirements
- M5Stack Core (BASIC, GRAY, GO, FIRE, Core2, Tough, or CoreS3)
- M5Stack DMX Base module
- 32-Channel 96A RGBW DMX 512 LED Controller
- LED strips connected to your DMX controller
- WiFi network

## Installation Steps

### 1. Install ESPHome
If you haven't already, install ESPHome:

**Option A: Home Assistant Add-on (Recommended)**
1. Go to Home Assistant → Settings → Add-ons
2. Add the ESPHome repository if not already added
3. Install the ESPHome add-on

**Option B: Standalone Installation**
```bash
pip install esphome
```

### 2. Prepare Configuration Files

1. **Copy the configuration file**: Place `m5stack-dmx-controller.yaml` in your ESPHome config directory

2. **Create secrets file**:
   ```bash
   cp secrets.yaml.template secrets.yaml
   ```

3. **Edit secrets.yaml** with your actual values:
   ```yaml
   wifi_ssid: "Your_WiFi_Network_Name"
   wifi_password: "your_wifi_password"
   api_encryption_key: "your_32_character_api_encryption_key_here"
   ota_password: "your_ota_password"
   fallback_password: "fallback_hotspot_password"
   ```

   **Generate API encryption key**:
   ```bash
   esphome wizard m5stack-dmx-controller.yaml
   ```
   Or use: `openssl rand -hex 32`

### 3. Hardware Setup

1. **Assemble M5Stack with DMX Base**:
   - Attach the DMX Base module to your M5Stack Core
   - Connect power to the DMX Base (DC 9-24V)

2. **Configure your DMX Controller**:
   - Set starting DMX address to **1** (using controller's buttons)
   - Set to **RGBW mode** (4-channel groups)
   - Choose **8-bit** or **16-bit** mode as desired
   - Connect XLR cable from M5Stack DMX Base to your controller

3. **Connect LED strips** to your 32-channel controller outputs

### 4. Flash the Firmware

**First Time Flash (USB Cable Required)**:
```bash
esphome run m5stack-dmx-controller.yaml
```

**Subsequent Updates (OTA)**:
```bash
esphome run m5stack-dmx-controller.yaml --device IP_ADDRESS
```

### 5. Add to Home Assistant

1. Go to Home Assistant → Settings → Devices & Services
2. ESPHome should auto-discover your device
3. Click "Configure" and enter your API encryption key
4. Your 8 DMX light groups will appear as entities

## Usage

### Home Assistant Control

You'll see 8 new light entities in Home Assistant:
- `light.dmx_group_1` through `light.dmx_group_8`

Each group provides:
- **RGBW Color Control**: Individual red, green, blue, and white channels
- **Brightness**: Overall brightness control
- **Effects**: Random, strobe, and flicker effects
- **On/Off**: Toggle each group independently

### Local M5Stack Controls

- **Button A**: Toggle Group 1 on/off
- **Button B**: Cycle through groups 1-8 and toggle the current group
- **Button C**: Turn off all groups

### Display Information

The M5Stack LCD shows:
- WiFi connection status and IP address
- Current DMX values for Groups 1 and 2
- Device status

## Channel Mapping

The configuration maps your 32 channels to 8 RGBW groups:

```
Group 1: DMX Ch 1-4   (Red, Green, Blue, White)
Group 2: DMX Ch 5-8   (Red, Green, Blue, White)
Group 3: DMX Ch 9-12  (Red, Green, Blue, White)
Group 4: DMX Ch 13-16 (Red, Green, Blue, White)
Group 5: DMX Ch 17-20 (Red, Green, Blue, White)
Group 6: DMX Ch 21-24 (Red, Green, Blue, White)
Group 7: DMX Ch 25-28 (Red, Green, Blue, White)
Group 8: DMX Ch 29-32 (Red, Green, Blue, White)
```

## Customization

### Modify Pin Configuration

If you have a different M5Stack model, update the UART pins in the YAML:

```yaml
uart:
  - id: dmx_uart
    tx_pin: 13  # Change for your M5Stack model
    rx_pin: 35  # Change for your M5Stack model
```

**Pin mapping by model**:
- **BASIC/GRAY/GO/FIRE**: TX=13, RX=35
- **Core2/Tough**: TX=19, RX=35  
- **CoreS3**: TX=7, RX=10

### Add More Effects

Add custom effects to any light group:

```yaml
light:
  - platform: rgbw
    name: "DMX Group 1"
    # ... other config ...
    effects:
      - random:
      - strobe:
      - flicker:
      - addressable_rainbow:
          name: "Rainbow"
          speed: 10
          width: 50
```

### Modify Display Content

Customize the display lambda to show different information:

```yaml
display:
  - platform: ili9341
    # ... other config ...
    lambda: |-
      it.fill(COLOR_BLACK);
      it.print(0, 0, id(font_medium), COLOR_WHITE, "My DMX Controller");
      // Add your custom display code here
```

## Troubleshooting

### WiFi Connection Issues
1. Check your `secrets.yaml` WiFi credentials
2. Look for the fallback hotspot "M5Stack-DMX-Controller" if main WiFi fails
3. Check signal strength in device location

### DMX Output Issues
1. Verify XLR wiring between M5Stack DMX Base and controller
2. Ensure DMX controller is set to starting address 1
3. Check that DMX controller is in RGBW mode
4. Verify power connections to both M5Stack and DMX controller

### Home Assistant Integration
1. Ensure API encryption key matches between YAML and Home Assistant
2. Check that ESPHome add-on is running in Home Assistant
3. Look for the device in Settings → Devices & Services → ESPHome

### Display Issues
1. If display is blank, check that you're using the correct M5Stack model in config
2. Verify SPI pin connections are correct for your M5Stack version

## Advanced Configuration

### Custom Automations

Create Home Assistant automations using your DMX lights:

```yaml
automation:
  - alias: "Colorful Welcome"
    trigger:
      platform: state
      entity_id: binary_sensor.front_door
      to: 'on'
    action:
      - service: light.turn_on
        target:
          entity_id: light.dmx_group_1
        data:
          rgb_color: [255, 0, 0]
          brightness: 255
      - delay: '00:00:01'
      - service: light.turn_on
        target:
          entity_id: light.dmx_group_2
        data:
          rgb_color: [0, 255, 0]
          brightness: 255
```

### Scene Control

Create scenes for different lighting moods:

```yaml
scene:
  - name: "Party Mode"
    entities:
      light.dmx_group_1:
        state: on
        effect: "random"
        brightness: 255
      light.dmx_group_2:
        state: on
        effect: "strobe"
        brightness: 200
```

## Support

For issues specific to:
- **ESPHome**: Check the [ESPHome documentation](https://esphome.io/)
- **M5Stack hardware**: Visit [M5Stack docs](https://docs.m5stack.com/)
- **DMX Base module**: See [M5Stack DMX Base](https://docs.m5stack.com/en/base/dmx)
- **Home Assistant**: Check [Home Assistant docs](https://www.home-assistant.io/docs/)

## License

This configuration is provided as-is for educational and personal use.