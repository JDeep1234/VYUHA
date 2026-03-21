# 🛡️ EDR Adaptive Framework

> **Advanced APT Simulation & EDR Evasion Assessment Platform**

A comprehensive C++ framework for testing and evaluating Endpoint Detection and Response (EDR) solutions through simulated Advanced Persistent Threat (APT) techniques, mapped to the MITRE ATT&CK framework.

---


## 🎯 Overview

The EDR Adaptive Framework is designed to help security teams:

- **Assess EDR Effectiveness**: Test how well your EDR solution detects various attack techniques
- **Simulate APT Campaigns**: Execute realistic attack chains mapped to the Cyber Kill Chain
- **Identify Detection Gaps**: Discover blind spots in your security posture
- **Generate Compliance Reports**: Produce detailed reports with MITRE ATT&CK mappings

---

## 📊 Project Status (March 2026)

| Component | Owner | Status | Progress |
|-----------|-------|--------|----------|
| **CLI, Agent Core, Integration** | Jdeep | ✅ Complete | 100% |
| **BYOVD Exploit (T1068)** | Bipin | ✅ Complete | 100% |
| **EDR-Freeze Exploit (T1562.001)** | Bipin | ✅ Complete | 100% |
| **Crystal Palace UDRL (T1055.001)** | Bipin | ✅ Complete | 100% |
| **ML Framework** | Karthik | ✅ Complete | 100% |

**Overall Framework Status:** ✅ Fully operational — all three modules complete and integrated end-to-end

**Current Capabilities:**
- ✅ Fully interactive TUI with exploit execution
- ✅ BYOVD kernel-level EDR process termination (T1068)
- ✅ EDR-Freeze user-mode process suspension (T1562.001)
- ✅ Crystal Palace UDRL — 6-layer EDR evasion chain with reflective DLL loading (T1055.001)
- ✅ Auto-detection of 60+ EDR products
- ✅ VM snapshot management (Hyper-V, VirtualBox, VMware)
- ✅ Telemetry and artifact cleanup systems
- ✅ ML-based adaptive attack strategy (DQN + SHAP + K-means, fully integrated)

---

## 🏗️ Architecture

The framework follows a modular C++ architecture with distinct layers:

<img width="1596" height="1072" alt="image" src="https://github.com/user-attachments/assets/8706fbd0-6f73-4fb6-afdf-3c3273fd7bea" />



---

## 📁 Project Structure

```
EDR-Adaptive-Framework/
├── CMakeLists.txt              # Main build configuration
├── README.md
├── config/
│   └── config.yaml             # Framework configuration
│
├── include/                    # Header files
│   ├── cli/
│   │   └── cli.hpp             # CLI interface [Jdeep]
│   ├── agent_core/
│   │   └── agent.hpp           # Agent core [Jdeep]
│   ├── integration/
│   │   └── integration_manager.hpp  # EDR/VM integration [Jdeep]
│   ├── exploits/
│   │   ├── exploit_manager.hpp # Exploit techniques [BIPIN]
│   │   ├── byovd_vulndriver.hpp
│   │   ├── edr_freeze.hpp
│   │   └── crystal_palace_loader.hpp
│   └── ml_framework/
│       └── ml_engine.hpp       # ML analysis [KARTHIK]
│
├── src/                        # Source files
│   ├── main.cpp                # Entry point
│   ├── cli/                    # CLI implementation [Jdeep]
│   ├── agent_core/             # Agent core [Jdeep]
│   ├── integration/            # Integration module [Jdeep]
│   ├── exploits/               # Exploit scripts [BIPIN]
│   └── ml_framework/           # ML framework [KARTHIK]
│
└── docs/
    └── architecture.drawio     # Architecture diagram
```

---

## 🚀 Building

### Prerequisites

- **CMake** 3.16+
- **C++17** compatible compiler (MSVC, GCC, Clang)
- **Windows SDK** (for Windows API)

### Build Steps

```bash
# Clone repository
git clone https://github.com/JDeep1234/EDR-Adaptive-Framework.git
cd EDR-Adaptive-Framework

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# Run
./bin/edr_framework.exe --help
```

---

## 📖 Usage

### Basic Commands

```bash
# Execute single technique
edr_framework run -t T1055 --verbose

# Run attack campaign
edr_framework campaign -c attacks.yaml -o html

# List available techniques
edr_framework list

# Check EDR status
edr_framework status

# Create VM snapshot
edr_framework snapshot -s clean_state

# Cleanup artifacts
edr_framework clean --all
```

### Campaign File Example (attacks.yaml)

```yaml
name: "APT Simulation Campaign"
techniques:
  - id: T1055
    name: "Process Injection"
    options:
      target: notepad.exe
      
  - id: T1574.002
    name: "DLL Side-Loading"
    options:
      app: Teams

  - id: T1218.005
    name: "Mshta Execution"
```

---

## 🎯 MITRE ATT&CK Coverage

### ✅ Implemented Techniques

| Technique ID | Name | Tactic | Status | Owner |
|--------------|------|--------|--------|-------|
| **T1068** | **BYOVD - Exploitation for Privilege Escalation** | **Defense Evasion, Privilege Escalation** | **✅ 100%** | **Bipin** |
| **T1562.001** | **EDR-Freeze - Impair Defenses (Process Suspension)** | **Defense Evasion** | **✅ 100%** | **Bipin** |
| **T1055.001** | **Crystal Palace UDRL - Reflective DLL Loading** | **Defense Evasion, Execution** | **✅ 100%** | **Bipin** |

---

## 🔧 Module Details

### 🟢 Jdeep's Modules (CLI, Agent Core, Integration) - ✅ 100% Complete

**Status:** Fully implemented and tested

**CLI Tool (`src/cli/`)**
- ✅ Interactive TUI with color-coded menus
- ✅ Exploit selection and execution interface
- ✅ Campaign management system
- ✅ Snapshot integration controls
- ✅ Real-time progress indicators

**Agent Core (`src/agent_core/`)**
- ✅ `Orchestrator`: Execution state machine
- ✅ `OutputHandler`: JSON, CSV, HTML, STIX export
- ✅ `Cleaner`: Artifact removal (files, registry, services)
- ✅ `TelemetryMonitor`: EDR event monitoring

**Integration (`src/integration/`)**
- ✅ `EDRConnector`: CrowdStrike, Defender, Carbon Black APIs
- ✅ `SnapshotManager`: Hyper-V, VirtualBox, VMware support
- ✅ `CleanModule`: System backup and restore

### 🟠 Bipin's Module (Exploit Scripts) - ✅ 3/3 Techniques Complete

**Location:** `src/exploits/`

**Development Status:**
- **3 Major Techniques Implemented**
- **3 Techniques Fully Implemented and Tested (100%)**

#### ✅ Implemented: BYOVD (Bring Your Own Vulnerable Driver) - T1068

**Status:** 100% Complete and Tested

**Exploit Flow:**

```
┌─────────────────────────────────────────────────────────────┐
│  BYOVD Attack Chain - CVE-2023-52271 (vulndriver.sys)      │
└─────────────────────────────────────────────────────────────┘

[1/6] Enable SeLoadDriverPrivilege
      ↓ Obtain permission to load kernel drivers
      
[2/6] Deploy Driver (vulndriver.sys → TEMP)
      ↓ Copy signed driver to system temp directory
      
[3/6] Create Windows Service ("WarsawPMDriver")
      ↓ Register driver as kernel service
      
[4/6] Start Driver
      ↓ Load vulnerable driver into kernel space
      
[5/6] Open Device Handle (\\.\Warsaw_PM)
      ↓ Establish communication with kernel driver
      
[6/6] Test Kernel Access (IOCTL 0x22201C)
      ↓ Verify ZwTerminateProcess capability
      
[*] Interactive Target Selection:
    • Option 1: Auto-detect EDR processes (60+ signatures)
      - Scans all running processes
      - Identifies EDR agents (Defender, CrowdStrike, etc.)
      - Displays PIDs and full paths
      - User selects target from list
      
    • Option 2: Manual PID entry
      - User provides specific process ID
      - Direct termination via kernel IOCTL
      
    • Option 3: Load driver only
      - Establishes kernel access without killing
      - Leaves driver loaded for manual operations
      
[KILL] terminateProcess(pid)
       ↓ Send IOCTL 0x22201C with target PID
       ↓ Driver calls ZwTerminateProcess from kernel
       ✓ Process terminated (bypasses EDR hooks)

[CLEANUP] Automatic artifact removal
          ↓ Close device handle
          ↓ Stop and delete service
          ↓ Remove driver file
```

**Capabilities:**
- ✅ Interactive driver path configuration
- ✅ Automatic EDR process detection (60+ vendors)
- ✅ Kernel-level process termination (bypasses PPL/PPE)
- ✅ Three operating modes (auto/manual/load-only)
- ✅ Complete cleanup on exit

**Supported EDR Detection:**
- Microsoft Defender, CrowdStrike Falcon, SentinelOne
- Carbon Black, Symantec, McAfee/Trellix, Trend Micro
- Kaspersky, ESET, Sophos, Palo Alto, Cylance
- FireEye, Fortinet, Cisco AMP, Webroot
- *Full list: 60+ process signatures*

**Technical Details:**
- **CVE:** CVE-2023-52271
- **Driver:** vulndriver.sys (wsftprm.sys)
- **Signer:** TPZ SOLUCOES DIGITAIS LTDA
- **Blocklist Status:** NOT blocklisted by Microsoft (as of 2026)
- **IOCTL Code:** 0x22201C (1036-byte buffer, PID in first 4 bytes)
- **Kernel Function:** ZwTerminateProcess
- **Device Object:** `\\.\Warsaw_PM`

📖 **Full Documentation:** [BYOVD_USAGE.md](contexts/BYOVD_USAGE.md)

---

#### ✅ Implemented: EDR-Freeze (Process Suspension Deadlock) - T1562.001

**Status:** 100% Complete and Tested

**Exploit Flow:**

```
┌─────────────────────────────────────────────────────────────┐
│  EDR-Freeze Attack Chain - WerFaultSecure Race Condition   │
└─────────────────────────────────────────────────────────────┘

[1/4] Enable SeDebugPrivilege
      ↓ Obtain permission for process manipulation
      
[2/4] Identify Target EDR Process
      ↓ Auto-detect or manual PID selection
      ↓ Resolve main thread ID (via NtQuerySystemInformation)
      
[3/4] Launch WerFaultSecure.exe as PPL (WinTCB)
      ↓ CreateProcess with PROC_THREAD_ATTRIBUTE_PROTECTION_LEVEL
      ↓ Command: /h /pid <TARGET_PID> /tid <TARGET_TID> ...
      ↓ MiniDumpWriteDump() suspends ALL target threads
      
[4/4] Win the Race - Monitor & Freeze
      ↓ Spin-wait loop detects target suspension
      ↓ Call NtSuspendProcess(WerFaultSecure)
      ✓ DEADLOCK: EDR waiting for dump, WerFault suspended
      ✓ EDR enters "coma state" (all threads suspended)
      
[HOLD] Sleep for configured duration (default: 10 seconds)
       ↓ EDR remains unresponsive, security monitoring disabled
       
[RELEASE] Terminate WerFaultSecure
          ↓ EDR process resumes normal operation
          ↓ Delete dump file artifact
```

**Capabilities:**
- ✅ User-mode only (no driver required)
- ✅ Living-off-the-Land (uses Windows component)
- ✅ Reversible freeze (EDR resumes after release)
- ✅ Auto-detection of EDR processes
- ✅ Configurable freeze duration
- ✅ Automatic artifact cleanup

**Supported Targets:**
- Microsoft Defender (MsMpEng.exe, MsSense.exe)
- CrowdStrike Falcon (CSFalconService.exe) 
- SentinelOne (SentinelAgent.exe)
- Carbon Black (CbDefense.exe)
- Any process with active threads (PPL-protected or not)

**Technical Details:**
- **Technique:** Race condition exploitation
- **Component:** WerFaultSecure.exe (Windows Error Reporting)
- **Protection Level:** WinTCB (PPL level 0)
- **Suspend Mechanism:** MiniDumpWriteDump thread suspension
- **Deadlock:** NtSuspendProcess on WerFaultSecure
- **Detection:** Behavioral only (no signature-based)

**Advantages over BYOVD:**
- ✅ No kernel driver required
- ✅ Legitimate Windows binary (LOLBIN)
- ✅ Reversible (EDR not killed, just frozen)
- ✅ No blocklisted drivers
- ✅ Harder to detect (no driver load telemetry)

📖 **Full Documentation:** [EDR_FREEZE_USAGE.md](contexts/EDR_FREEZE_USAGE.md)

---

#### ✅ Implemented: Crystal Palace UDRL (Reflective DLL Loading) - T1055.001

**Status:** 100% Complete and Tested

**Based on:** [KaplaStrike](https://github.com/kapla0011/KaplaStrike) by @kapla  
**Technical Reference:** [Bypassing EDR in a Crystal Clear Way](https://lorenzomeacci.com/bypassing-edr-in-a-crystal-clear-way)

**Exploit Flow:**

```
┌─────────────────────────────────────────────────────────────┐
│  Crystal Palace UDRL - 6-Layer EDR Evasion Chain           │
│  Self-Contained Position-Independent Code (PIC) Blob       │
└─────────────────────────────────────────────────────────────┘

[PREP] Build PIC blob with KaplaStrike:
       cd KaplaStrike && make x64
       ./link spec/loader.spec beacon.dll output.bin
       ↓ Produces self-contained blob with all evasion layers

[Step 1/3] Read blob from disk
           ↓ Load output.bin into process memory

[Step 2/3] Allocate executable memory (RW → RX)
           ↓ VirtualAlloc PAGE_READWRITE, copy blob
           ↓ VirtualProtect → PAGE_EXECUTE_READ
           ↓ (Avoids RWX IOC that EDRs flag)

[Step 3/3] Call blob entry point: go()
           ↓ NtContinue transfers execution (does NOT return)
           ↓ Process becomes the beacon

── Inside the blob, 6 evasion layers fire automatically: ──

  Layer 1: XOR Payload Decryption
           ↓ findAppendedDLL() + findMask()
           ↓ Rolling XOR decrypts beacon DLL in-place

  Layer 2: Module Overloading
           ↓ NtCreateSection + NtMapViewOfSection
           ↓ Overwrites legitimate DLL (WsmSvc.dll) in memory
           ↓ EDR sees backed (legitimate) memory, not shellcode

  Layer 3: .pdata Registration
           ↓ RtlAddFunctionTable registers exception handlers
           ↓ Stack unwinding looks legitimate to EDR

  Layer 4: NtContinue Entry Transfer
           ↓ Synthetic BaseThreadInitThunk stack frames
           ↓ RtlUserThreadStart spoofed origin
           ↓ Thread appears to originate from kernel32.dll

  Layer 5: API Call Stack Spoofing (Draugr)
           ↓ Every Win32 API call proxied via gadget
           ↓ archiveint.dll gadget masks true caller
           ↓ EDR stack traces show legitimate DLL frames

  Layer 6: Sleep Masking
           ↓ IAT-hooked Sleep() function
           ↓ XOR-encrypts beacon memory during sleep
           ↓ Periodic memory scans find only encrypted bytes

  ✓ Beacon running with full C2 capability
  ✓ EDR monitoring shows zero alerts
```

**Capabilities:**
- ✅ Evades static signature scanning (XOR-encrypted payload on disk)
- ✅ Evades memory scanning (Module Overloading + Sleep masking)
- ✅ Evades stack trace analysis (NtContinue + Draugr spoofing)
- ✅ No RWX memory pages (RW→RX two-step allocation)
- ✅ Self-contained PIC blob — zero framework-side evasion code
- ✅ Works with custom DLL payloads (reverse shells, C2 beacons)
- ✅ Living-off-the-Land memory components (WsmSvc.dll, archiveint.dll)

**Tested Against:**
- Microsoft Defender (Windows 10/11) — ✅ Evaded
- Static analysis — ✅ Evaded (XOR encryption)
- Memory scanning — ✅ Evaded (Module Overloading)

**Technical Details:**
- **MITRE ATT&CK:** T1055.001 (Process Injection: DLL Injection via RDLL)
- **Blob Format:** Crystal Palace PIC (Position-Independent Code)
- **Sections:** `[loader code] [PICO code] [cobalt_dll (XOR)] [cobalt_mask]`
- **Entry Point:** `go()` at blob offset 0
- **Key APIs:** NtCreateSection, NtMapViewOfSection, NtContinue, RtlAddFunctionTable
- **Sacrificial DLL:** WsmSvc.dll (Module Overloading target)
- **Spoofing Gadget:** archiveint.dll (Draugr call stack spoofing)

**Evasion Comparison:**

| Attack | Method | Impact | Stealth |
|--------|--------|--------|--------|
| **BYOVD** | Kill EDR process via kernel driver | EDR goes offline | ⚠️ SOC sees sensor offline |
| **EDR-Freeze** | Suspend EDR via WerFault deadlock | EDR frozen temporarily | ⚠️ EDR resumes after release |
| **Crystal Palace** | Invisible C2 beacon behind 6 evasion layers | Persistent silent access | ✅ EDR stays running, zero alerts |

📖 **Full Documentation:** [CRYSTAL_PALACE_USAGE.md](contexts/CRYSTAL_PALACE_USAGE.md)

---

**How to Add New Exploit:**
```cpp
// In src/exploits/your_exploit.cpp
class YourExploit : public BaseExploit {
public:
    ExploitResult execute(const std::map<std::string, std::string>& options) override {
        // Your implementation
    }
    
    TechniqueInfo getInfo() const override {
        // Return MITRE info
    }
};
```

### 🟣 Karthik's Module (ML Framework) - ✅ 100% Complete

**Location:** `src/ml_framework/`

**Status:** Fully implemented and integrated with Jdeep's agent loop and Bipin's exploit engine

**Completed Components:**
- ✅ `ml_engine.cpp` — Full `analyze()`, `recommendAction()`, `generateReport()`, `loadModels()`, `saveModels()`
- ✅ `ml_bridge.cpp` — Windows `CreateProcess` subprocess bridge (JSON stdio, no pybind11 required)
- ✅ `detection_analyzer.cpp` — 22-keyword weighted EDR alert detection
- ✅ `evasion_scorer.cpp` — Rolling per-technique stealth scoring
- ✅ `event_correlator.cpp` — MITRE ATT&CK correlation (10 technique mappings)
- ✅ `python/strategy_selector.py` — DQN agent (state=30, actions=8, experience replay, target network)
- ✅ `python/behavior_analyzer.py` — K-means EDR clustering (n=4), observation accumulation
- ✅ `python/explainable_ai.py` — SHAP KernelExplainer + gradient fallback, human-readable narratives
- ✅ `python/adaptive_learner.py` — Online learning, transfer learning, failure pattern blacklisting
- ✅ `python/ml_server.py` — stdio JSON server (10 commands: select_action, train, explain, cluster, etc.)
- ✅ `python/utils.py` — Shared constants: FEATURE_NAMES[30], ACTIONS[8], reward helpers, state builder
- ✅ `python/train_agent.py` — Standalone training CLI with SimulatedEDREnv and transfer learning
- ✅ `data/edr_profiles.json` — 6 EDR seed profiles for K-means initialisation
- ✅ `data/execution_logs.csv` — Seed execution log data

**Integration:**
```
CLI::cmdCampaign()
  └── AgentCore::runCampaign(file, exploits, mlEngine)
        ├── mlEngine.recommendAction(state)     ← DQN selects technique
        ├── exploitManager.execute(techniqueId) ← Bipin's real Win32 exploits
        └── mlEngine.analyze(result, s, s')     ← 3 analyzers + online train
```

**See detailed design:** [`src/ml_framework/DESIGN.md`](src/ml_framework/DESIGN.md)

---

## 🔒 Supported EDRs

| EDR | Integration Status |
|-----|-------------------|
| Microsoft Defender | ✅ Implemented |
| CrowdStrike Falcon | 🔄 API Ready |
| VMware Carbon Black | 🔄 API Ready |
| SentinelOne | 📋 Planned |
| Sophos | 📋 Planned |

---

## 🖥️ VM Providers

| Provider | Status |
|----------|--------|
| Hyper-V | ✅ Implemented |
| VirtualBox | ✅ Implemented |
| VMware Workstation | ✅ Implemented |

---



