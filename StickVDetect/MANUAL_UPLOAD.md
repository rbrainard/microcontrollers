# Manual Upload Instructions for M5StickV

If automated upload isn't working, you can manually upload files using MaixPy IDE or serial terminal.

## Method 1: Using MaixPy IDE (Recommended)

### Step 1: Download MaixPy IDE
- Download from: http://dl.sipeed.com/MAIX/MaixPy/ide/
- Install and run MaixPy IDE

### Step 2: Connect
1. Open MaixPy IDE
2. Click **Tools → Select Board → M5StickV**
3. Click **Tools → Select Port → COM12** (your port)
4. Click **Connect** button (green plug icon)

### Step 3: Upload Files
For each file (config.py, boot.py, main.py):
1. Open the file in MaixPy IDE
2. Click **Tools → Send file to device**
3. Save as `/flash/filename.py` on the device

### Step 4: Run
1. Close MaixPy IDE connection
2. Reset M5StickV (power cycle)
3. It will auto-run main.py

---

## Method 2: Using Serial Terminal (Advanced)

### Step 1: Connect with PuTTY or similar
- Port: COM12
- Baud: 115200
- Data: 8 bits, No parity, 1 stop bit

### Step 2: Enter REPL mode
1. Press Ctrl+C to stop any running program
2. You should see `>>>` prompt

### Step 3: Paste file contents
For each file, type:
```python
f = open('/flash/config.py', 'w')
f.write('''
# Paste entire contents of config.py here
''')
f.close()
```

Repeat for boot.py and main.py

### Step 4: Reset
Press Ctrl+D to soft reboot

---

## Method 3: Direct Copy via MicroPython

### Step 1: Test REPL access
```powershell
python -m serial.tools.miniterm COM12 115200
```

### Step 2: At >>> prompt, type:
```python
import os
os.listdir('/flash')
```

If this works, you have REPL access!

### Step 3: Upload files manually
Use the paste method from Method 2 above.

---

## Troubleshooting

### "Device not found" or "Port busy"
1. Reset M5StickV (hold power 6s, power on again)
2. Close all serial programs (Arduino IDE, PuTTY, etc.)
3. Try different USB cable (must be data cable, not charge-only)

### "No >>> prompt"
The device might be running a program:
1. Press Ctrl+C several times
2. If still no prompt, may need to reflash firmware

### Reflash MaixPy Firmware
If nothing works, the device may need firmware:

1. **Download kflash_gui:**
   ```powershell
   pip install kflash_gui
   ```

2. **Download firmware:**
   - Visit: https://dl.sipeed.com/shareURL/MAIX/MaixPy/release/master
   - Download: `maixpy_v*_m5stickv.bin`

3. **Flash:**
   ```powershell
   kflash_gui
   ```
   - Select firmware file
   - Select COM12
   - Click Download
   - Wait ~30 seconds

4. **After flashing:**
   - M5StickV will reboot
   - Should show MaixPy boot screen
   - Now try upload methods above

---

## Quick Test Script

Save this as `test.py` and manually paste into REPL to test WiFi:

```python
import network
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect("Hacienda", "brainframe")
import time
time.sleep(5)
print(wlan.ifconfig())
```

If this prints an IP address, WiFi is working!

---

## Alternative: SD Card Method

If all else fails:
1. Copy config.py, boot.py, main.py to microSD card
2. Insert SD card into M5StickV
3. Files will be in `/sd/` instead of `/flash/`
4. Edit main.py to import from correct path

---

## Need More Help?

- M5StickV Docs: https://docs.m5stack.com/en/core/m5stickv
- MaixPy Forum: https://bbs.sipeed.com/
- M5Stack Discord: https://discord.gg/m5stack
