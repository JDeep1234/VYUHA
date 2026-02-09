@echo off
REM ============================================================================
REM EDR Adaptive Framework - Windows Build Script
REM ============================================================================
REM Run this on Windows 10 VM with Visual Studio 2022 installed
REM ============================================================================

echo.
echo ========================================
echo EDR Adaptive Framework - Build Script
echo ========================================
echo.

REM Check for Administrator privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [!] ERROR: Administrator privileges required
    echo     Right-click and "Run as Administrator"
    pause
    exit /b 1
)
echo [+] Running as Administrator

REM Check for CMake
where cmake >nul 2>&1
if %errorLevel% neq 0 (
    echo [!] ERROR: CMake not found
    echo     Install CMake from https://cmake.org/download/
    pause
    exit /b 1
)
echo [+] CMake found

REM Check for Visual Studio
where cl.exe >nul 2>&1
if %errorLevel% neq 0 (
    echo [!] ERROR: Visual Studio compiler not found
    echo.
    echo     Run this from "Developer Command Prompt for VS 2022"
    echo     OR run: "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    pause
    exit /b 1
)
echo [+] Visual Studio compiler found

REM Create build directory
if exist build (
    echo [*] Cleaning previous build...
    rmdir /s /q build
)

echo [+] Creating build directory...
mkdir build
cd build

REM Configure with CMake
echo.
echo [*] Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorLevel% neq 0 (
    echo [!] CMake configuration failed
    pause
    exit /b 1
)
echo [+] CMake configuration successful

REM Build
echo.
echo [*] Building (Release mode)...
cmake --build . --config Release -j 8
if %errorLevel% neq 0 (
    echo [!] Build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Binary location: %CD%\bin\Release\edr_framework.exe
echo.

REM Copy driver to build directory
if exist ..\vulndriver.sys (
    echo [*] Copying vulndriver.sys to build directory...
    copy ..\vulndriver.sys bin\Release\
    echo [+] Driver copied
)

echo.
echo Ready for testing! Run:
echo     cd build\bin\Release
echo     edr_framework.exe
echo.

pause
