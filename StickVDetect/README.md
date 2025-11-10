# M5StickV Human Detection & Motor Tracking

This project uses the M5StickV AI camera to detect humans and automatically position the 485Brushless motor to track them via REST API.

## 🚀 Quick Start

**Windows:** Double-click `upload.bat`  
**Linux/Mac:** Run `./upload.sh`

See [QUICKSTART.md](QUICKSTART.md) for detailed instructions.

## Hardware Required
- M5StickV (K210 AI Camera)
- M5Stack Station 485 with Unit Roller485 motor
- WiFi network connecting both devices

## How It Works
1. M5StickV continuously captures images and runs human detection
2. When a human is detected, calculates their position in the frame (X, Y coordinates)
3. Converts the position to motor angle
4. Sends REST API command to the motor controller to point at the detected person
5. Displays detection info on the M5StickV screen

## Setup Instructions

### 1. Install Python Dependencies
```bash
pip install -r requirements.txt
```

### 2. Configure WiFi and Motor
Edit `config.py`:
```python
WIFI_SSID = "YourNetwork"       # Your WiFi network (2.4GHz)
WIFI_PASSWORD = "YourPassword"  # Your WiFi password
MOTOR_IP = "192.168.86.31"      # IP of your motor controller
```

### 3. Upload to M5StickV
**Automatic (recommended):**
```bash
# Windows
upload.bat

# Linux/Mac
./upload.sh
```

**Manual:**
```bash
ampy --port COM3 put config.py
ampy --port COM3 put boot.py
ampy --port COM3 put main.py
```

### 4. Run
- Power on M5StickV - auto-runs main.py on boot
- Press Button A to start/stop tracking
- Press Button B to toggle debug display

> **Note:** M5StickV must have MaixPy firmware. See [install_guide.md](install_guide.md) for firmware flashing.

## Motor Positioning
- The motor will position based on detected human's X coordinate
- Pan range: -90° to +90° (left to right)
- Detection area is divided into segments for smooth tracking
- Dead zone in center prevents jitter

## Detection Models
The code includes three detection options:
1. **Motion Detection** (default) - Best for stationary camera, detects any movement
2. **Face Detection** - Faster, works well for tracking people facing camera
3. **YOLO Object Detection** - Most accurate, can detect full body (requires model)

Edit `config.py` to change detection mode:
```python
USE_YOLO = False    # True = YOLO, False = use USE_FACE setting
USE_FACE = False    # True = Face detection, False = Motion detection
MOTION_THRESHOLD = 25      # Lower = more sensitive motion detection
MOTION_MIN_AREA = 500      # Minimum pixels to trigger detection
```

## Troubleshooting
- **No WiFi connection**: Check SSID/password, ensure 2.4GHz network
- **Motor not moving**: Verify motor IP address, check network connectivity
- **Poor detection**: Adjust lighting, ensure person is 0.5-3m from camera
- **Screen issues**: Check LCD initialization, try power cycle

## API Endpoints Used
- `GET /api/status` - Check motor status
- `POST /api/control/position` - Move motor to position

## File Structure
```
StickVDetect/
├── README.md           # Project overview
├── QUICKSTART.md       # Quick start guide
├── install_guide.md    # Detailed installation guide
├── requirements.txt    # Python dependencies
├── upload.py          # Python upload script
├── upload.bat         # Windows upload script
├── upload.sh          # Linux/Mac upload script
├── config.py          # Configuration settings (EDIT THIS!)
├── boot.py            # Boot screen for M5StickV
├── main.py            # Main detection and tracking code
└── .gitignore         # Git ignore file
```

Files uploaded to M5StickV:
- `config.py` - WiFi and tracking settings
- `boot.py` - Boot screen
- `main.py` - Main program

## Performance
- Detection FPS: ~10-15 fps
- Tracking update rate: ~5 Hz
- Network latency: ~50-100ms
- Total tracking delay: ~200-300ms

## Safety Notes
- Motor movement is limited to safe ranges
- Dead zone prevents continuous micro-adjustments
- Timeout stops motor if no detection for 5 seconds
- Emergency stop via button combination (A+B)
