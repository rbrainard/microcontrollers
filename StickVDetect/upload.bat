@echo off
REM Windows batch script to upload M5StickV Human Tracker
REM Usage: upload.bat [COMPORT]
REM Example: upload.bat COM3

echo ========================================
echo M5StickV Human Tracker Upload Script
echo ========================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python not found!
    echo Please install Python from https://www.python.org/
    pause
    exit /b 1
)

REM Check if ampy is installed
pip show adafruit-ampy >nul 2>&1
if errorlevel 1 (
    echo ampy not found. Installing...
    pip install adafruit-ampy
    if errorlevel 1 (
        echo Error: Failed to install ampy
        pause
        exit /b 1
    )
)

REM Run the upload script
if "%1"=="" (
    python upload.py --verify
) else (
    python upload.py --port %1 --verify
)

if errorlevel 1 (
    echo.
    echo Upload failed!
    pause
    exit /b 1
)

echo.
echo Upload successful!
pause
