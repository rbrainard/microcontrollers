"""
M5StickV Face Tracking for Turret
Centers detected face and sends angle via UART
Based on official MaixPy face detection examples
"""

import sensor, image, lcd, time
from machine import UART

# Init camera
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)  # 320x240
sensor.set_vflip(0)
sensor.set_hmirror(0)
sensor.run(1)

# Init LCD
lcd.init()
lcd.rotation(2)

# UART for sending tracking data
uart = UART(UART.UART1, 115200, 8, None, 1, timeout=1000, read_buf_len=4096)

print("Face Tracker Started")

# Face detection settings
face_cascade = image.HaarCascade("frontalface", stages=25)
FRAME_CENTER = 160  # Center X of 320px frame
DEAD_ZONE = 20      # Don't move if within ±20 pixels of center

def pixel_to_angle(pixel_x):
    """Convert pixel X to motor angle (-90 to +90)"""
    offset = pixel_x - FRAME_CENTER
    angle = (offset / FRAME_CENTER) * 90
    return max(-90, min(90, int(angle)))

clock = time.clock()

while True:
    clock.tick()
    img = sensor.snapshot()
    
    # Detect faces
    faces = img.find_features(face_cascade, threshold=0.5, scale_factor=1.5)
    
    if faces:
        # Track largest face (closest person)
        largest_face = max(faces, key=lambda f: f[2] * f[3])
        x, y, w, h = largest_face
        
        # Calculate center
        face_center_x = x + w // 2
        face_center_y = y + h // 2
        
        # Calculate error from frame center
        error_x = face_center_x - FRAME_CENTER
        
        # ALWAYS draw bounding box and center point
        img.draw_rectangle(x, y, w, h, color=(0, 255, 0), thickness=2)
        img.draw_circle(face_center_x, face_center_y, 5, color=(255, 0, 0), thickness=2)
        
        # Draw all detected faces (not just largest) in yellow
        for face in faces:
            fx, fy, fw, fh = face
            if face != largest_face:
                img.draw_rectangle(fx, fy, fw, fh, color=(255, 255, 0), thickness=1)
        
        # Calculate motor angle
        angle = pixel_to_angle(face_center_x)
        
        # Send UART command
        if abs(error_x) > DEAD_ZONE:
            uart.write("TRACK,{},{},{}\n".format(angle, face_center_x, face_center_y))
            img.draw_string(5, 5, "TRACK", color=(0, 255, 0))
            img.draw_string(5, 20, "Ang:{}".format(angle), color=(255, 255, 255))
        else:
            uart.write("CENTER\n")
            img.draw_string(5, 5, "CENTERED", color=(0, 255, 255))
        
        print("Face at X={} Error={} Angle={}°".format(face_center_x, error_x, angle))
    else:
        uart.write("SEARCH\n")
        img.draw_string(5, 5, "Searching...", color=(255, 255, 0))
    
    # Draw center crosshair
    img.draw_line(FRAME_CENTER - 10, 120, FRAME_CENTER + 10, 120, color=(255, 255, 0))
    img.draw_line(FRAME_CENTER, 110, FRAME_CENTER, 130, color=(255, 255, 0))
    
    # Show FPS
    fps = clock.fps()
    img.draw_string(5, 220, "FPS:{:.1f}".format(fps), color=(255, 255, 255))
    
    lcd.display(img)
