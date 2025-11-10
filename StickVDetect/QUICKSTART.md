# Quick Start Guide

## For Windows Users

### 1. Install Python Tools
```powershell
cd StickVDetect
pip install -r requirements.txt
```

### 2. Edit Configuration
Open `config.py` and update:
- `WIFI_SSID` - Your WiFi network name
- `WIFI_PASSWORD` - Your WiFi password
- `MOTOR_IP` - Your motor controller IP address (default: "192.168.86.31")

### 3. Upload to M5StickV
Simply double-click `upload.bat` or run:
```powershell
.\upload.bat
```

The script will:
- Auto-detect your M5StickV COM port
- Upload all files (config.py, boot.py, main.py)
- Verify the upload
- Show you next steps

### 4. Manual Upload (if needed)
```powershell
# Find COM port in Device Manager, then:
python -m ampy.cli --port COM3 put config.py
python -m ampy.cli --port COM3 put boot.py
python -m ampy.cli --port COM3 put main.py
```

### 5. Test
- Power cycle M5StickV
- Wait for "Ready!" on screen
- Press Button A to start tracking
- Press Button B to toggle debug view

---

## For Linux/Mac Users

### 1. Install Python Tools
```bash
cd StickVDetect
pip3 install -r requirements.txt
```

### 2. Edit Configuration
Open `config.py` and update WiFi and motor settings.

### 3. Upload to M5StickV
```bash
chmod +x upload.sh
./upload.sh
```

Or specify port manually:
```bash
./upload.sh /dev/ttyUSB0
```

### 4. Manual Upload (if needed)
```bash
# Find port with: ls /dev/tty*
python -m ampy.cli --port /dev/ttyUSB0 put config.py
python -m ampy.cli --port /dev/ttyUSB0 put boot.py
python -m ampy.cli --port /dev/ttyUSB0 put main.py
```

---

## Troubleshooting

### COM Port Not Found
**Windows:** Check Device Manager → Ports (COM & LPT)
**Linux/Mac:** Run `ls /dev/tty*` to find your device

### Permission Denied (Linux/Mac)
```bash
sudo usermod -a -G dialout $USER
# Then log out and back in
```

### Upload Fails
1. Make sure M5StickV is powered on
2. Close any serial monitor programs
3. Try unplugging and replugging USB cable
4. Specify port manually: `upload.bat COM3` or `./upload.sh /dev/ttyUSB0`

### WiFi Connection Fails
- Ensure WiFi is 2.4GHz (M5StickV doesn't support 5GHz)
- Check SSID and password in config.py
- Move closer to WiFi router

---

## Using the Tracker

### Button Controls
- **Button A** - Toggle tracking on/off
- **Button B** - Toggle debug display (shows detection boxes)

### Screen Display
- **STANDBY** (red) - Tracking disabled
- **TRACKING** (green) - Actively tracking
- **FPS** - Camera frame rate
- **Pos** - Current motor position in degrees

### Monitoring Serial Output
```powershell
# Windows
python -m ampy.cli --port COM3 run main.py

# Linux/Mac
python -m ampy.cli --port /dev/ttyUSB0 run main.py
```

---

## Configuration Tips

Edit `config.py` to tune performance:

### For Faster Tracking
```python
UPDATE_INTERVAL = 100        # Update every 100ms
POSITION_SMOOTHING = 0.2     # Less smoothing
MIN_MOVE_ANGLE = 1          # More sensitive
```

### For Smoother Tracking
```python
UPDATE_INTERVAL = 300        # Update every 300ms
POSITION_SMOOTHING = 0.5     # More smoothing
MIN_MOVE_ANGLE = 3          # Less sensitive
DEAD_ZONE = 15              # Bigger dead zone
```

### For Better Detection
```python
DETECTION_THRESHOLD = 0.5    # Lower = more detections
CAMERA_HMIRROR = True       # Flip image if needed
CAMERA_VFLIP = True         # Flip image if needed
```

---

## Full Documentation

- **README.md** - Project overview
- **install_guide.md** - Detailed installation and troubleshooting
- **config.py** - All configuration options with comments

---

## Need Help?

Check the full installation guide: `install_guide.md`

For M5StickV documentation: https://docs.m5stack.com/en/core/m5stickv
