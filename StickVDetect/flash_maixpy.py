"""
Flash MaixPy Firmware to M5StickV
Step-by-step firmware flashing tool
"""

import os
import sys
import subprocess
import urllib.request

FIRMWARE_URL = "https://dl.sipeed.com/shareURL/MAIX/MaixPy/release/master/maixpy_v0.6.3"
FIRMWARE_FILE = "maixpy_v0.6.3_86_gc3f26350f_m5stickv.bin"

def check_kflash():
    """Check if kflash is installed"""
    try:
        result = subprocess.run(['kflash', '--help'], 
                              capture_output=True, timeout=5)
        return True
    except:
        return False

def install_kflash():
    """Install kflash"""
    print("Installing kflash...")
    result = subprocess.run(['pip', 'install', 'kflash'], 
                          capture_output=True, text=True)
    if result.returncode == 0:
        print("✓ kflash installed")
        return True
    else:
        print("✗ Failed to install kflash")
        print(result.stderr)
        return False

def download_firmware():
    """Download MaixPy firmware"""
    if os.path.exists(FIRMWARE_FILE):
        print(f"✓ Firmware already downloaded: {FIRMWARE_FILE}")
        return True
    
    print(f"Downloading MaixPy firmware...")
    print(f"From: {FIRMWARE_URL}/{FIRMWARE_FILE}")
    
    try:
        url = f"{FIRMWARE_URL}/{FIRMWARE_FILE}"
        urllib.request.urlretrieve(url, FIRMWARE_FILE)
        print(f"✓ Downloaded: {FIRMWARE_FILE}")
        return True
    except Exception as e:
        print(f"✗ Download failed: {e}")
        print("\nManual download:")
        print(f"1. Visit: {FIRMWARE_URL}")
        print(f"2. Download: {FIRMWARE_FILE}")
        print(f"3. Place in: {os.getcwd()}")
        return False

def flash_firmware(port):
    """Flash firmware to M5StickV"""
    if not os.path.exists(FIRMWARE_FILE):
        print(f"✗ Firmware file not found: {FIRMWARE_FILE}")
        return False
    
    print("\n" + "="*60)
    print("FLASHING FIRMWARE - DO NOT DISCONNECT!")
    print("="*60)
    print(f"Port: {port}")
    print(f"Firmware: {FIRMWARE_FILE}")
    print("\nThis will take about 30-60 seconds...")
    print("You'll see progress bars as it flashes")
    print("="*60 + "\n")
    
    try:
        # Flash using kflash
        result = subprocess.run([
            'kflash',
            '-p', port,
            '-b', '1500000',  # Fast baud rate
            FIRMWARE_FILE
        ])
        
        if result.returncode == 0:
            print("\n" + "="*60)
            print("✓ FIRMWARE FLASHED SUCCESSFULLY!")
            print("="*60)
            return True
        else:
            print("\n✗ Flashing failed")
            return False
            
    except Exception as e:
        print(f"\n✗ Error during flash: {e}")
        return False

def main():
    print("="*60)
    print("M5StickV MaixPy Firmware Flasher")
    print("="*60)
    print("\nThis will replace EasyLoader with MaixPy firmware")
    print("MaixPy is required to run Python code on M5StickV\n")
    
    # Check kflash
    if not check_kflash():
        print("kflash not found")
        if not install_kflash():
            print("\nManual installation:")
            print("  pip install kflash")
            return 1
    
    # Check if firmware exists
    if not os.path.exists(FIRMWARE_FILE):
        print("="*60)
        print("FIRMWARE DOWNLOAD REQUIRED")
        print("="*60)
        print("\nAutomatic download doesn't work for this file.")
        print("Please download manually:\n")
        print("1. Open browser and visit:")
        print("   https://dl.sipeed.com/shareURL/MAIX/MaixPy/release/master/maixpy_v0.6.3")
        print("\n2. Find and click:")
        print("   maixpy_v0.6.3_86_gc3f26350f_m5stickv.bin")
        print("\n3. Save the file to:")
        print(f"   {os.getcwd()}")
        print("\n4. Then run this script again")
        print("="*60)
        
        # Try to open browser
        import webbrowser
        try:
            webbrowser.open("https://dl.sipeed.com/shareURL/MAIX/MaixPy/release/master/maixpy_v0.6.3")
            print("\n✓ Opening browser...")
        except:
            pass
        
        return 1
    
    # Get COM port
    port = input("\nEnter COM port (e.g., COM12): ").strip()
    
    # Confirm
    print(f"\n⚠️  Ready to flash MaixPy to {port}")
    confirm = input("Continue? (yes/no): ").strip().lower()
    
    if confirm != 'yes':
        print("Cancelled")
        return 0
    
    # Flash
    if flash_firmware(port):
        print("\n" + "="*60)
        print("NEXT STEPS:")
        print("="*60)
        print("1. M5StickV should reboot and show MaixPy boot screen")
        print("2. Wait a few seconds for boot to complete")
        print("3. Run upload script:")
        print("   python upload.py")
        print("\nIf screen is blank:")
        print("- Press power button to wake")
        print("- Try power cycling (hold 6s, then power on)")
        print("="*60)
        return 0
    else:
        print("\n" + "="*60)
        print("TROUBLESHOOTING:")
        print("="*60)
        print("1. Make sure M5StickV is connected via USB")
        print("2. Close all serial programs")
        print("3. Try holding the reset button during flash")
        print("4. Try different USB cable")
        print("5. Manual flash with kflash_gui:")
        print("   - Download: https://github.com/sipeed/kflash_gui")
        print("   - Use GUI to select port and firmware file")
        print("="*60)
        return 1

if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\nFlashing cancelled")
        sys.exit(1)
