# M5StickV Installation Guide

## Prerequisites

### Hardware
- M5StickV (with K210 chip)
- USB-C cable
- M5Stack Station 485 with Unit Roller485 (already set up)
- WiFi router (2.4GHz network)

### Software
- Python 3.7+ (for ampy tool)
- kflash_gui (for firmware flashing)
- Serial terminal (optional, for debugging)

## Step-by-Step Installation

### 1. Install MaixPy Firmware (if needed)

If your M5StickV doesn't have MaixPy or you want the latest version:

1. Download kflash_gui:
   ```bash
   pip install kflash_gui
   ```

2. Download latest MaixPy firmware:
   - Visit: https://dl.sipeed.com/shareURL/MAIX/MaixPy/release/master
   - Download: `maixpy_v*_m5stickv.bin`

3. Flash firmware:
   ```bash
   # Connect M5StickV, find COM port
   kflash_gui
   # Select firmware file, select COM port, click Download
   ```

### 2. Install ampy (for file transfer)

```bash
pip install adafruit-ampy
```

### 3. Configure Your Settings

Edit `config.py`:
```python
# Your WiFi network (must be 2.4GHz)
WIFI_SSID = "YourNetworkName"
WIFI_PASSWORD = "YourPassword"

# Your motor controller IP address
MOTOR_IP = "192.168.86.31"  # Find this from your motor controller
```

### 4. Upload Files to M5StickV

Find your COM port:
- Windows: Check Device Manager → Ports (COM & LPT)
- Mac/Linux: `ls /dev/tty.*` or `ls /dev/ttyUSB*`

Upload files:
```bash
# Navigate to StickVDetect folder
cd StickVDetect

# Upload configuration
python -m ampy.cli --port COM3 put config.py

# Upload main program
python -m ampy.cli --port COM3 put main.py

# Optional: Upload boot.py for custom boot screen
python -m ampy.cli --port COM3 put boot.py
```

### 5. Verify Installation

Connect to M5StickV serial console:
```bash
# Windows
python -m ampy.cli --port COM3 run --no-output main.py

# Or use a serial terminal
# Baud: 115200, Data: 8, Parity: None, Stop: 1
```

You should see:
```
M5StickV Boot Complete
Starting main.py...
========================================
M5StickV Human Tracker
========================================
LCD initialized
Camera initialized
Connecting to YourNetwork...
Connected! IP: 192.168.86.XX
```

### 6. Test the System

1. **Power on M5StickV** - it auto-runs main.py
2. **Check LCD display** - should show "Ready!"
3. **Press Button A** - enables tracking (screen shows "TRACKING")
4. **Stand in front of camera** - motor should move to track you
5. **Press Button B** - toggles debug overlay (detection boxes)

## Troubleshooting

### Issue: "WiFi Failed!"
**Solution:**
- Verify WiFi is 2.4GHz (StickV doesn't support 5GHz)
- Check SSID and password in config.py
- Ensure WiFi network is available
- Try moving closer to router

### Issue: "Motor Error!"
**Solution:**
- Verify motor controller IP address
- Check motor controller is powered and connected to same WiFi
- Test motor controller API: `curl http://192.168.86.31/api/status`
- Check firewall isn't blocking connections

### Issue: Camera not working
**Solution:**
- Check camera lens isn't covered
- Verify MaixPy firmware is properly installed
- Try power cycling the M5StickV
- Check serial output for camera initialization errors

### Issue: Poor detection
**Solution:**
- Ensure good lighting (not backlit)
- Stay within 0.5-3 meters from camera
- Face the camera directly
- Adjust `DETECTION_THRESHOLD` in config.py (lower = more sensitive)

### Issue: Motor moves too fast/slow
**Solution:**
- Adjust `UPDATE_INTERVAL` in config.py (higher = slower updates)
- Adjust `POSITION_SMOOTHING` (0.0-1.0, higher = smoother but slower)
- Change `MOTOR_SPEED` value

### Issue: Motor jitters/oscillates
**Solution:**
- Increase `DEAD_ZONE` (prevents small movements)
- Increase `MIN_MOVE_ANGLE` (minimum angle before moving)
- Increase `POSITION_SMOOTHING` (0.5-0.7 recommended)

## Useful Commands

### List files on M5StickV
```bash
python -m ampy.cli --port COM3 ls
```

### View a file
```bash
python -m ampy.cli --port COM3 get main.py
```

### Delete a file
```bash
python -m ampy.cli --port COM3 rm main.py
```

### Get serial output
```bash
python -m ampy.cli --port COM3 run main.py
```

### Reset M5StickV
Press the power button (side button) for 6 seconds

## Advanced Configuration

### Using YOLO Detection

In `config.py`, set:
```python
USE_YOLO = True
```

Note: This requires YOLO model to be loaded on M5StickV. See MaixPy documentation for model loading.

### Adding Tilt Control

If you add a second motor for tilt:
1. Modify `send_motor_command()` to send tilt angle
2. Add Y-axis detection tracking
3. Adjust `TILT_MIN` and `TILT_MAX` in config.py

### Custom Detection Models

MaixPy supports custom KPU models. To use your own:
1. Train model using MaixHub or Sipeed tools
2. Convert to .kmodel format
3. Load in main.py using `KPU.load()`

## Performance Tuning

For best performance:
- Use face detection (faster than YOLO)
- Keep `UPDATE_INTERVAL` at 200ms or higher
- Set `POSITION_SMOOTHING` to 0.3-0.5
- Ensure good lighting
- Keep camera 1-2 meters from subject

## Safety Notes

⚠️ **Important Safety Considerations:**
- Motor has limited range (±90°) to prevent cable tangling
- Dead zone prevents continuous micro-movements
- Motor stops if no detection for 5 seconds
- Both buttons pressed = emergency stop (future feature)
- Keep hands clear of motor during operation

## Support

For issues or questions:
- M5StickV Docs: https://docs.m5stack.com/en/core/m5stickv
- MaixPy Docs: https://wiki.sipeed.com/maixpy
- M5Stack Community: https://community.m5stack.com
