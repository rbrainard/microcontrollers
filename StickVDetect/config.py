# M5StickV Human Tracking Configuration
# Edit these settings to match your setup

# WiFi Configuration
WIFI_SSID = "Hacienda"
WIFI_PASSWORD = "brainframe"

# Motor Controller Configuration
MOTOR_IP = "192.168.86.31"
MOTOR_PORT = 80

# Tracking Parameters
PAN_MIN = -90        # Minimum pan angle (degrees)
PAN_MAX = 90         # Maximum pan angle (degrees)
TILT_MIN = -30       # Minimum tilt angle (degrees) - if you add tilt motor
TILT_MAX = 30        # Maximum tilt angle (degrees)

# Detection Settings
DETECTION_THRESHOLD = 0.6    # Confidence threshold for detections
UPDATE_INTERVAL = 200        # Milliseconds between motor updates
DEAD_ZONE = 10              # Pixels - dead zone in center to prevent jitter
NO_DETECTION_TIMEOUT = 5000  # ms - return to center if no detection

# Camera Settings
CAMERA_WIDTH = 320
CAMERA_HEIGHT = 240
CAMERA_HMIRROR = False      # Horizontal mirror
CAMERA_VFLIP = False        # Vertical flip

# Display Settings
SHOW_DEBUG = True           # Show detection boxes and info
SHOW_FPS = True            # Show FPS counter
LCD_ROTATION = 2           # LCD rotation (0, 1, 2, 3)

# Position Calculation
# These convert pixel position to motor position
# Assumes camera at 0° points straight ahead
# X position maps to motor angle
POSITION_SCALE = (PAN_MAX - PAN_MIN) / CAMERA_WIDTH  # degrees per pixel

# Smoothing
POSITION_SMOOTHING = 0.3    # 0.0 = no smoothing, 1.0 = max smoothing
MIN_MOVE_ANGLE = 2          # Minimum angle change to send command

# Motor Settings
MOTOR_SPEED = 1000          # Motor speed for position moves (encoder units)
MOTOR_MAX_CURRENT = 100000  # Maximum current (100000 = 1A)

# Detection Mode
USE_YOLO = False           # True = YOLO detection, False = Motion detection
USE_FACE = False           # True = Face detection, False = Motion detection (ignored if USE_YOLO=True)
MOTION_THRESHOLD = 25      # Sensitivity for motion detection (lower = more sensitive)
MOTION_MIN_AREA = 500      # Minimum blob size to detect
YOLO_CLASSES = [14]        # COCO classes to detect (14 = person)

# Colors (RGB565)
COLOR_GREEN = (0, 255, 0)
COLOR_RED = (255, 0, 0)
COLOR_BLUE = (0, 0, 255)
COLOR_YELLOW = (255, 255, 0)
COLOR_WHITE = (255, 255, 255)
COLOR_BLACK = (0, 0, 0)
