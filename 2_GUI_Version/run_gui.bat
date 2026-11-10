@echo off
REM Compiles SmartParkingPro.cpp and opens the window.
cd /d "%~dp0"
echo Compiling SmartParkingPro.cpp ...
g++ SmartParkingPro.cpp -o SmartParkingPro.exe -mwindows -lgdi32
if errorlevel 1 (
    echo.
    echo Build failed. Check the errors above.
    pause
    exit /b 1
)
start "" SmartParkingPro.exe
