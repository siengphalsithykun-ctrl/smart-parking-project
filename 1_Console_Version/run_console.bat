@echo off
REM Compiles SmartParking.cpp and runs it in this terminal window.
cd /d "%~dp0"
echo Compiling SmartParking.cpp ...
g++ SmartParking.cpp -o SmartParking.exe
if errorlevel 1 (
    echo.
    echo Build failed. Check the errors above.
    pause
    exit /b 1
)
echo Done. Starting the program...
echo.
SmartParking.exe
echo.
pause
