"""
Super simple motion detection test
"""

import sensor, image, lcd, time
from Maix import GPIO
from fpioa_manager import fm
from board import board_info

# Button setup
fm.register(board_info.BUTTON_A, fm.fpioa.GPIO1)
button_a = GPIO(GPIO.GPIO1, GPIO.IN, GPIO.PULL_UP)

# Init
lcd.init()
lcd.rotation(2)
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(30)

print("Ready! Press Button A to toggle tracking")

tracking = False
ref_img = None
frame_count = 0

while True:
    # Check button
    if button_a.value() == 0:
        time.sleep_ms(100)
        if button_a.value() == 0:
            tracking = not tracking
            ref_img = None
            print("Tracking: {}".format("ON" if tracking else "OFF"))
            while button_a.value() == 0:
                time.sleep_ms(10)

    # Get frame
    img = sensor.snapshot()

    # Motion detection
    if tracking:
        frame_count += 1

        # Get new reference every 10 frames
        if ref_img is None or frame_count > 10:
            ref_img = sensor.snapshot()
            frame_count = 0
            img.draw_string(5, 5, "REF", color=(255, 255, 0))
        else:
            # Compare stats
            try:
                curr_mean = img.get_statistics().l_mean()
                ref_mean = ref_img.get_statistics().l_mean()
                diff = abs(curr_mean - ref_mean)

                if diff > 15:
                    # Motion!
                    img.draw_string(5, 5, "MOTION!", color=(0, 255, 0))
                    img.draw_rectangle(80, 60, 160, 120, color=(0, 255, 0), thickness=2)
                    print("Motion: diff={}".format(diff))
                else:
                    img.draw_string(5, 5, "TRACKING", color=(0, 255, 0))
            except Exception as e:
                print("Error: {}".format(e))
                ref_img = None
    else:
        img.draw_string(5, 5, "STANDBY", color=(255, 0, 0))

    lcd.display(img)
    time.sleep_ms(30)
