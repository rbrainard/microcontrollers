#!/bin/bash
# Linux/Mac upload script for M5StickV Human Tracker
# Usage: ./upload.sh [PORT]
# Example: ./upload.sh /dev/ttyUSB0

echo "========================================"
echo "M5StickV Human Tracker Upload Script"
echo "========================================"
echo

# Check if Python is installed
if ! command -v python3 &> /dev/null; then
    echo "Error: Python not found!"
    echo "Please install Python 3"
    exit 1
fi

# Check if ampy is installed
if ! pip3 show adafruit-ampy &> /dev/null; then
    echo "ampy not found. Installing..."
    pip3 install adafruit-ampy
    if [ $? -ne 0 ]; then
        echo "Error: Failed to install ampy"
        exit 1
    fi
fi

# Run the upload script
if [ -z "$1" ]; then
    python3 upload.py --verify
else
    python3 upload.py --port "$1" --verify
fi

if [ $? -ne 0 ]; then
    echo
    echo "Upload failed!"
    exit 1
fi

echo
echo "Upload successful!"
