"""
Absolute minimal test - just hello world + text overlay
"""

import sensor, image, lcd, time
from Maix import GPIO
from fpioa_manager import fm
from board import board_info

# Button setup
fm.register(board_info.BUTTON_A, fm.fpioa.GPIO1)
button_a = GPIO(GPIO.GPIO1, GPIO.IN, GPIO.PULL_UP)

lcd.init()
lcd.rotation(2)
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(30)

print("Running minimal test...")

counter = 0
button_pressed = False
prev_pixel = 0

while True:
    img = sensor.snapshot()
    
    # Check button
    if button_a.value() == 0:
        button_pressed = True
        print("Button pressed!")
    
    # Sample a pixel
    pixel = img.get_pixel(160, 120)
    pixel_value = pixel[0]  # Red channel
    
    if prev_pixel == 0:
        prev_pixel = pixel_value
    
    diff = abs(pixel_value - prev_pixel)
    
    # Just draw text - no other operations
    status = "PRESSED" if button_pressed else "Ready"
    img.draw_string(5, 5, "Test {} {} px:{}".format(counter, status, diff), color=(0, 255, 0))
    
    lcd.display(img)
    
    counter += 1
    if counter % 30 == 0:
        print("Counter: {}".format(counter))
    
    time.sleep_ms(30)
