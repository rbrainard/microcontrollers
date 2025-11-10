# M5StickV Boot Configuration
# This file runs automatically when M5StickV powers on

import lcd
import sys

# Initialize LCD
lcd.init()
lcd.rotation(2)
lcd.clear()

# Display boot message
lcd.draw_string(10, 10, "M5StickV", lcd.WHITE, lcd.BLACK)
lcd.draw_string(10, 30, "Human Tracker", lcd.WHITE, lcd.BLACK)
lcd.draw_string(10, 50, "Initializing...", lcd.GREEN, lcd.BLACK)

# Optional: Set CPU frequency for performance
# import machine
# machine.freq(400000000)  # 400MHz

print("M5StickV Boot Complete")
print("Starting main.py...")
