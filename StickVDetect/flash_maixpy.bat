@echo off
REM Flash MaixPy firmware to M5StickV

echo ========================================
echo M5StickV MaixPy Firmware Flasher
echo ========================================
echo.
echo This will replace EasyLoader with MaixPy
echo MaixPy is required to run Python code
echo.

python flash_maixpy.py

pause
