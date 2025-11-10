"""
Test M5StickV Connection
Quick diagnostic tool to check if M5StickV is responding
"""

import sys
import subprocess
from serial.tools import list_ports

def find_ports():
    """List all available COM ports"""
    print("Available COM ports:")
    ports = list_ports.comports()
    for p in ports:
        print(f"  {p.device}: {p.description}")
    return ports

def test_connection(port):
    """Test if we can connect to the port"""
    print(f"\nTesting connection to {port}...")
    
    try:
        # Try to list files (quick test)
        print("Attempting to list files (10 second timeout)...")
        result = subprocess.run(
            ['python', '-m', 'ampy.cli', '--port', port, '--baud', '115200', 'ls'],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        if result.returncode == 0:
            print("✓ Connection successful!")
            print("Files on device:")
            print(result.stdout)
            return True
        else:
            print("✗ Connection failed")
            print(f"Error: {result.stderr}")
            return False
            
    except subprocess.TimeoutExpired:
        print("✗ Connection timeout - device not responding")
        return False
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

def main():
    print("="*50)
    print("M5StickV Connection Test")
    print("="*50)
    
    # List ports
    ports = find_ports()
    
    if not ports:
        print("\n✗ No COM ports found!")
        print("Please connect M5StickV via USB")
        return 1
    
    # Auto-detect or ask user
    if len(ports) == 1:
        port = ports[0].device
        print(f"\nAuto-detected: {port}")
    else:
        port = input("\nEnter COM port (e.g., COM12): ").strip()
    
    # Test connection
    if test_connection(port):
        print("\n" + "="*50)
        print("✓ M5StickV is ready for upload!")
        print("="*50)
        print("\nYou can now run: python upload.py")
        return 0
    else:
        print("\n" + "="*50)
        print("✗ M5StickV not responding")
        print("="*50)
        print("\nTroubleshooting steps:")
        print("1. Reset M5StickV:")
        print("   - Hold power button for 6 seconds until it turns off")
        print("   - Press power button again to turn on")
        print("   - Wait for boot screen")
        print("\n2. Check the screen:")
        print("   - Should show M5StickV logo or boot text")
        print("   - If blank, try charging or different USB cable")
        print("\n3. Try MaixPy IDE:")
        print("   - Download: http://dl.sipeed.com/MAIX/MaixPy/ide/")
        print("   - Connect and see if IDE can detect it")
        print("\n4. Re-flash firmware:")
        print("   - If all else fails, may need to reflash MaixPy firmware")
        return 1

if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\nTest cancelled")
        sys.exit(1)
