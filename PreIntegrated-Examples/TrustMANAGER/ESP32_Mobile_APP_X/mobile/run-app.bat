@echo off
echo ========================================
echo keySTREAM Provisioning App - Quick Test
echo ========================================
echo.

cd /d "%~dp0"

echo [1/5] Checking Node.js...
node -v
if %errorlevel% neq 0 (
    echo ERROR: Node.js not found!
    pause
    exit /b 1
)

echo [2/5] Checking npm...
npm.cmd -v
if %errorlevel% neq 0 (
    echo ERROR: npm not found!
    pause
    exit /b 1
)

echo [3/5] Checking React Native dependencies...
if not exist "node_modules" (
    echo ERROR: Dependencies not installed!
    echo Run: npm install
    pause
    exit /b 1
)

echo [4/5] Checking for connected Android device...
adb devices
echo.
echo Make sure you see your device listed above!
echo.

echo [5/5] All checks passed!
echo.
echo ========================================
echo Ready to run! Choose an option:
echo ========================================
echo 1. Start Metro bundler only (npm start)
echo 2. Build and run on Android (npm run android)
echo 3. Exit
echo.
set /p choice="Enter your choice (1-3): "

if "%choice%"=="1" (
    echo.
    echo Starting Metro bundler...
    npm.cmd start
) else if "%choice%"=="2" (
    echo.
    echo Building and running on Android...
    echo This may take a few minutes...
    npm.cmd run android
) else (
    echo Exiting...
)

pause
