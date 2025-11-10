"""
M5StickV Motion Detection Demo
Simple motion tracking without WiFi - just displays motion on screen
"""

import sensor
import image
import lcd
import time
from Maix import GPIO
from fpioa_manager import fm
from board import board_info

# Settings
CAMERA_WIDTH = 320
CAMERA_HEIGHT = 240
MOTION_THRESHOLD = 25
MOTION_MIN_AREA = 500
SHOW_DEBUG = True
DETECTION_SKIP = 5  # Only detect every 5 frames to prevent freezing

# Global state
tracking_enabled = False
previous_img = None
frame_skip_count = 0  # Track frames since last capture

# Button setup
fm.register(board_info.BUTTON_A, fm.fpioa.GPIO1)
fm.register(board_info.BUTTON_B, fm.fpioa.GPIO2)
button_a = GPIO(GPIO.GPIO1, GPIO.IN, GPIO.PULL_UP)
button_b = GPIO(GPIO.GPIO2, GPIO.IN, GPIO.PULL_UP)

def init_lcd():
    lcd.init()
    lcd.rotation(2)
    lcd.clear()
    lcd.draw_string(10, 10, "Motion Tracker", lcd.WHITE, lcd.BLACK)
    print("LCD initialized")

def init_camera():
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.set_hmirror(0)
    sensor.set_vflip(0)
    sensor.run(1)
    sensor.skip_frames(30)
    print("Camera initialized")

def detect_motion(img, threshold=25, min_area=500):
    global previous_img, frame_skip_count
    
    print("detect_motion: Start")
    
    # Capture reference frame every 10 frames
    frame_skip_count += 1
    if previous_img is None or frame_skip_count > 10:
        print("detect_motion: Capturing new reference frame")
        previous_img = sensor.snapshot()
        frame_skip_count = 0
        print("detect_motion: Reference captured")
        return None
    
    try:
        print("detect_motion: Getting stats...")
        # Use statistics instead of copy
        current_stats = img.get_statistics()
        prev_stats = previous_img.get_statistics()
        
        # Compare mean values
        diff = abs(current_stats.l_mean() - prev_stats.l_mean())
        print("detect_motion: Stats diff = {}".format(diff))
        
        if diff > threshold:
            print("detect_motion: Motion detected!")
            # Return center of frame as "detection"
            x = CAMERA_WIDTH // 4
            y = CAMERA_HEIGHT // 4
            w = CAMERA_WIDTH // 2
            h = CAMERA_HEIGHT // 2
            return (x, y, w, h)
        
        print("detect_motion: No motion")
    except Exception as e:
        print("detect_motion: ERROR - {}".format(e))
    
    print("detect_motion: End")
    return None

def draw_detection(img, detection):
    if detection:
        x, y, w, h = detection
        img.draw_rectangle(x, y, w, h, color=(0, 255, 0), thickness=2)
        center_x = x + w // 2
        center_y = y + h // 2
        img.draw_circle(center_x, center_y, 5, color=(255, 0, 0), thickness=2)

def draw_status(img, fps, tracking):
    status = "TRACKING" if tracking else "STANDBY"
    color = (0, 255, 0) if tracking else (255, 0, 0)
    img.draw_string(5, 5, status, color=color, scale=1)
    img.draw_string(5, 20, "FPS:{:.1f}".format(fps), color=(255, 255, 255), scale=1)
    
    # Crosshair
    center_x = CAMERA_WIDTH // 2
    center_y = CAMERA_HEIGHT // 2
    img.draw_line(center_x - 10, center_y, center_x + 10, center_y, color=(255, 255, 0))
    img.draw_line(center_x, center_y - 10, center_x, center_y + 10, color=(255, 255, 0))

def check_buttons():
    global tracking_enabled, SHOW_DEBUG
    
    if button_a.value() == 0:
        time.sleep_ms(50)
        if button_a.value() == 0:
            tracking_enabled = not tracking_enabled
            status = "ON" if tracking_enabled else "OFF"
            print("Tracking: {}".format(status))
            while button_a.value() == 0:
                time.sleep_ms(10)
    
    if button_b.value() == 0:
        time.sleep_ms(50)
        if button_b.value() == 0:
            SHOW_DEBUG = not SHOW_DEBUG
            print("Debug: {}".format(SHOW_DEBUG))
            while button_b.value() == 0:
                time.sleep_ms(10)

def main():
    print("="*40)
    print("M5StickV Motion Tracker")
    print("="*40)
    
    init_lcd()
    init_camera()
    
    lcd.clear()
    lcd.draw_string(10, 10, "Ready!", lcd.GREEN, lcd.BLACK)
    lcd.draw_string(10, 30, "A: Track On/Off", lcd.WHITE, lcd.BLACK)
    lcd.draw_string(10, 50, "B: Debug On/Off", lcd.WHITE, lcd.BLACK)
    time.sleep(2)
    
    fps_counter = 0
    fps_time = time.ticks_ms()
    current_fps = 0
    frame_count = 0
    
    while True:
        img = sensor.snapshot()
        check_buttons()
        
        # FPS
        fps_counter += 1
        if time.ticks_diff(time.ticks_ms(), fps_time) > 1000:
            current_fps = fps_counter
            fps_counter = 0
            fps_time = time.ticks_ms()
        
        detection = None
        if tracking_enabled:
            frame_count += 1
            print("Main loop: Frame count = {}".format(frame_count))
            if frame_count >= DETECTION_SKIP:
                print("Main loop: Calling detect_motion...")
                frame_count = 0
                detection = detect_motion(img, MOTION_THRESHOLD, MOTION_MIN_AREA)
                print("Main loop: detect_motion returned")
            
            if detection and SHOW_DEBUG:
                draw_detection(img, detection)
                x, y, w, h = detection
                center_x = x + w // 2
                print("Motion at X={}".format(center_x))
        
        draw_status(img, current_fps, tracking_enabled)
        lcd.display(img)
        time.sleep_ms(10)

if __name__ == "__main__":
    main()
