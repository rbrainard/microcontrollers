"""
Upload Script for M5StickV Human Tracker
Automatically uploads all necessary files to M5StickV
"""

import os
import sys
import time
import argparse
import subprocess
from serial.tools import list_ports

def find_stickv_port():
    """Auto-detect M5StickV COM port"""
    ports = list_ports.comports()
    
    # Look for common M5StickV identifiers
    for port in ports:
        if any(x in port.description.upper() for x in ['CH340', 'CP210', 'USB SERIAL', 'UART']):
            return port.device
    
    # If not found, return first available port
    if ports:
        print(f"Warning: M5StickV not auto-detected. Using {ports[0].device}")
        return ports[0].device
    
    return None

def upload_file(port, filename):
    """Upload a single file to M5StickV"""
    print(f"Uploading {filename}...")
    # Use python -m ampy instead of ampy command to avoid PATH issues
    cmd = ['python', '-m', 'ampy.cli', '--port', port, 'put', filename]
    
    try:
        # Use subprocess with timeout to prevent hanging
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        
        if result.returncode == 0:
            print(f"✓ {filename} uploaded successfully")
            return True
        else:
            print(f"✗ Failed to upload {filename}")
            if result.stderr:
                print(f"  Error: {result.stderr.strip()}")
            return False
    except subprocess.TimeoutExpired:
        print(f"✗ Upload timeout for {filename} (>30s)")
        print("  Try: Reset M5StickV and run again")
        return False
    except Exception as e:
        print(f"✗ Error uploading {filename}: {e}")
        return False

def list_stickv_files(port):
    """List files on M5StickV"""
    print("\nFiles on M5StickV:")
    try:
        result = subprocess.run(['python', '-m', 'ampy.cli', '--port', port, 'ls'], 
                              capture_output=True, text=True, timeout=10)
        if result.returncode == 0:
            print(result.stdout)
        else:
            print("Failed to list files")
    except Exception as e:
        print(f"Error listing files: {e}")

def main():
    parser = argparse.ArgumentParser(description='Upload M5StickV Human Tracker')
    parser.add_argument('--port', '-p', help='COM port (auto-detected if not specified)')
    parser.add_argument('--list', '-l', action='store_true', help='List files on device')
    parser.add_argument('--verify', '-v', action='store_true', help='Verify after upload')
    args = parser.parse_args()
    
    # Find COM port
    if args.port:
        port = args.port
    else:
        port = find_stickv_port()
        if not port:
            print("Error: No COM port found!")
            print("\nAvailable ports:")
            for p in list_ports.comports():
                print(f"  {p.device}: {p.description}")
            print("\nPlease specify port with --port COMX")
            return 1
    
    print(f"Using port: {port}")
    print("="*50)
    
    # Just list files if requested
    if args.list:
        list_stickv_files(port)
        return 0
    
    # Files to upload (in order)
    files_to_upload = [
        'config.py',    # Upload config first
        'boot.py',      # Boot screen
        'main.py',      # Main program last
    ]
    
    # Check all files exist
    missing_files = [f for f in files_to_upload if not os.path.exists(f)]
    if missing_files:
        print(f"Error: Missing files: {', '.join(missing_files)}")
        return 1
    
    # Upload each file
    print("\nStarting upload...")
    print("Note: Each file takes ~5-10 seconds")
    success_count = 0
    
    for filename in files_to_upload:
        if upload_file(port, filename):
            success_count += 1
            time.sleep(1)  # Brief delay between uploads
        else:
            print(f"\nUpload failed at {filename}")
            print("Troubleshooting:")
            print("  1. Reset M5StickV (hold power button 6 seconds)")
            print("  2. Check M5StickV is powered on and showing boot screen")
            print("  3. Try a different USB cable")
            print("  4. Close any serial monitor programs")
            print(f"  5. Verify correct port: {port}")
            return 1
    
    print("\n" + "="*50)
    print(f"✓ Upload complete! ({success_count}/{len(files_to_upload)} files)")
    
    # Verify if requested
    if args.verify:
        print("\nVerifying...")
        list_stickv_files(port)
    
    print("\n" + "="*50)
    print("Setup complete!")
    print("\nTo use the tracker:")
    print("  1. Power cycle the M5StickV (or press reset)")
    print("  2. Wait for 'Ready!' message on screen")
    print("  3. Press Button A to start/stop tracking")
    print("  4. Press Button B to toggle debug display")
    print("\nTo view serial output:")
    print(f"  python -m ampy.cli --port {port} run main.py")
    print("="*50)
    
    return 0

if __name__ == '__main__':
    # Change to script directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)
    
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\nUpload cancelled by user")
        sys.exit(1)
    except Exception as e:
        print(f"\nError: {e}")
        sys.exit(1)
