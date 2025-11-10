"""
Alternative upload method using rshell
More reliable than ampy for M5StickV
"""

import os
import sys
import subprocess

def check_rshell():
    """Check if rshell is installed"""
    try:
        result = subprocess.run(['rshell', '--version'], 
                              capture_output=True, text=True, timeout=5)
        return True
    except:
        return False

def install_rshell():
    """Install rshell"""
    print("Installing rshell...")
    result = subprocess.run(['pip', 'install', 'rshell'], 
                          capture_output=True, text=True)
    return result.returncode == 0

def upload_with_rshell(port):
    """Upload files using rshell"""
    files = ['config.py', 'boot.py', 'main.py']
    
    print(f"\nConnecting to {port}...")
    print("This will take a moment...\n")
    
    # Build rshell command
    commands = []
    for f in files:
        commands.append(f'cp {f} /flash/')
    
    cmd_str = '; '.join(commands)
    
    try:
        # Use rshell to copy files
        result = subprocess.run(
            ['rshell', '-p', port, '-b', '115200', cmd_str],
            timeout=60
        )
        
        if result.returncode == 0:
            print("\n✓ Upload successful!")
            return True
        else:
            print("\n✗ Upload failed")
            return False
            
    except subprocess.TimeoutExpired:
        print("\n✗ Upload timeout")
        return False
    except Exception as e:
        print(f"\n✗ Error: {e}")
        return False

def main():
    port = input("Enter COM port (e.g., COM12): ").strip()
    
    # Check if rshell is installed
    if not check_rshell():
        print("rshell not found. Installing...")
        if not install_rshell():
            print("Failed to install rshell")
            return 1
    
    print("\n" + "="*50)
    print("Uploading with rshell (alternative method)")
    print("="*50)
    
    if upload_with_rshell(port):
        print("\nFiles uploaded successfully!")
        print("Reset M5StickV to run the program")
        return 0
    else:
        print("\nUpload failed. See manual instructions below.")
        return 1

if __name__ == '__main__':
    sys.exit(main())
