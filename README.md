# VYUHA: Cross-Layer EDR Kill-Chain Evasion via Deep Reinforcement Learning-Guided Adversarial Orchestration

> Advanced APT Simulation and EDR Evasion Assessment Platform

VYUHA is a modular C++ framework for evaluating Endpoint Detection and Response (EDR) behavior through controlled ATT&CK-aligned simulation workflows.

---

## Overview

VYUHA helps security teams:

- Assess EDR effectiveness against different technique chains
- Simulate adversary-style workflows in a controlled lab
- Identify detection and telemetry gaps
- Export structured execution and analysis outputs

---

## 📊 Project Status (March 2026)

| Component | Owner | Status | Progress |
|-----------|-------|--------|----------|
| **CLI, Agent Core, Integration** | Jdeep | ✅ Complete | 100% |
| **BYOVD Exploit (T1068)** | Bipin | ✅ Complete | 100% |
| **EDR-Freeze Exploit (T1562.001)** | Bipin | ✅ Complete | 100% |
| **Crystal Palace UDRL (T1055.001)** | Bipin | ✅ Complete | 100% |
| **ML Framework** | Karthik | ✅ Complete | 100% |
## Project Status

| Component | Status | Notes |
|-----------|--------|-------|
| Core Framework (CLI, Agent Core, Integration) | Complete | Stable interactive workflow and orchestration |
| Exploit Modules | Active | BYOVD, EDR-Freeze, and Crystal Palace implemented; additional techniques planned |
| ML Framework | Complete | Adaptive recommendation and analysis pipeline integrated |

**Current Capabilities:**
- ✅ Fully interactive TUI with exploit execution
- ✅ BYOVD kernel-level EDR process termination (T1068)
- ✅ EDR-Freeze user-mode process suspension (T1562.001)
- ✅ Crystal Palace UDRL — 6-layer EDR evasion chain with reflective DLL loading (T1055.001)
- ✅ Auto-detection of 60+ EDR products
- ✅ VM snapshot management (Hyper-V, VirtualBox, VMware)
- ✅ Telemetry and artifact cleanup systems
- ✅ ML-based adaptive attack strategy (DQN + SHAP + K-means, fully integrated)
Overall status: Operational core with extensible module architecture.

---

## Architecture

The framework follows a layered modular architecture.

![Architecture Diagram](https://github.com/user-attachments/assets/8706fbd0-6f73-4fb6-afdf-3c3273fd7bea)

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
## Project Structure

```text
VYUHA/
|-- CMakeLists.txt
|-- README.md
|-- config/
|   `-- config.yaml
|-- include/
|   |-- cli/
|   |   `-- cli.hpp
|   |-- agent_core/
|   |   `-- agent.hpp
|   |-- integration/
|   |   `-- integration_manager.hpp
|   |-- exploits/
|   |   `-- exploit_manager.hpp
|   `-- ml_framework/
|       `-- ml_engine.hpp
|-- src/
|   |-- main.cpp
|   |-- cli/
|   |-- agent_core/
|   |-- integration/
|   |-- exploits/
|   `-- ml_framework/
`-- docs/
    `-- architecture.drawio
```

---

## Building

### Prerequisites

- CMake 3.16+
- C++17 compatible compiler (MSVC, GCC, Clang)
- Windows SDK (for Windows API components)

### Build Steps

```bash
# Clone repository
git clone <repository-url>
cd <project-directory>

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

## Usage

### Basic Commands

```bash
# Execute single technique
edr_framework run -t <technique-id> --verbose

# Run attack campaign
edr_framework campaign -c <campaign-file> -o <output-format>

# List available techniques
edr_framework list

# Check EDR status
edr_framework status

# Create VM snapshot
edr_framework snapshot -s <snapshot-name>

# Cleanup artifacts
edr_framework clean --all
```

### Campaign File Example

```yaml
name: "APT Simulation Campaign"
techniques:
  - id: T1055
    name: "Process Injection"
    options:
      target: "<target-process>"

  - id: T1574.002
    name: "DLL Side-Loading"
    options:
      app: "<target-application>"

  - id: T1218.005
    name: "Mshta Execution"
```

---

## MITRE ATT&CK Coverage

### Implemented Techniques

| Technique ID | Name | Tactic | Status | Owner |
|--------------|------|--------|--------|-------|
| **T1068** | **BYOVD - Exploitation for Privilege Escalation** | **Defense Evasion, Privilege Escalation** | **✅ 100%** | **Bipin** |
| **T1562.001** | **EDR-Freeze - Impair Defenses (Process Suspension)** | **Defense Evasion** | **✅ 100%** | **Bipin** |
| **T1055.001** | **Crystal Palace UDRL - Reflective DLL Loading** | **Defense Evasion, Execution** | **✅ 100%** | **Bipin** |
| Technique ID | Name | Tactic | Status |
|--------------|------|--------|--------|
| T1068 | BYOVD - Exploitation for Privilege Escalation | Defense Evasion, Privilege Escalation | Implemented |
| T1562.001 | EDR-Freeze - Impair Defenses (Process Suspension) | Defense Evasion | Implemented |
| T1055.001 | Crystal Palace UDRL - Dynamic-link Library Injection | Defense Evasion | Implemented |

### Planned Techniques

| Technique ID | Name | Tactic | Status |
|--------------|------|--------|--------|
| T1055 | Process Injection (additional variants) | Defense Evasion | Planned |
| T1218.002 | Control Panel | Defense Evasion | Planned |
| T1218.005 | Mshta | Defense Evasion | Planned |
| T1574.002 | DLL Side-Loading | Persistence | Planned |

---

## Module Details

### Core Modules (CLI, Agent Core, Integration)

### 🟠 Bipin's Module (Exploit Scripts) - ✅ 3/3 Techniques Complete
Status: Implemented and tested.

- CLI: interactive operation, command routing, technique orchestration
- Agent Core: execution orchestration, telemetry handling, artifact cleanup
- Integration: EDR connectors, snapshot manager, environment helpers

**Development Status:**
- **3 Major Techniques Implemented**
- **3 Techniques Fully Implemented and Tested (100%)**
### Exploit Modules

Status: Implemented techniques plus extensible interface for additional techniques.

Implemented:
- BYOVD (T1068)
- EDR-Freeze (T1562.001)
- Crystal Palace UDRL (T1055.001)

Planned:
- Process Injection (T1055)
- Control Panel (T1218.002)
- Mshta (T1218.005)
- DLL Side-Loading (T1574.002)

#### BYOVD (T1068)

- Driver-assisted process control workflow
- Configurable target selection (auto-detect or manual)
- Cleanup sequence for handles/services/artifacts

#### EDR-Freeze (T1562.001)

- User-mode process suspension workflow
- Configurable freeze duration and target selection
- Reversible execution path with cleanup support

#### Crystal Palace UDRL (T1055.001)

- Reflective loader workflow for staged payload execution
- Configurable blob path and execution options
- Integrated into the same exploit manager and campaign flow

### ML Framework

Status: Implemented and integrated with campaign execution flow.

Core components include:
- Scoring and analysis pipeline
- Strategy selection and adaptive learning
- Explainability helpers
- JSON-based Python bridge and model utilities

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
Integration flow:

```text
CLI command
  -> AgentCore::runCampaign(...)
     -> mlEngine.recommendAction(...)
     -> exploitManager.execute(...)
     -> mlEngine.analyze(...)
```

See detailed design: [src/ml_framework/DESIGN.md](src/ml_framework/DESIGN.md)

---

## Supported EDRs

| EDR | Integration Status |
|-----|--------------------|
| Microsoft Defender | Implemented |
| CrowdStrike Falcon | API-ready |
| VMware Carbon Black | API-ready |
| SentinelOne | Planned |
| Sophos | Planned |

---

## VM Providers

| Provider | Status |
|----------|--------|
| Hyper-V | Implemented |
| VirtualBox | Implemented |
| VMware Workstation | Implemented |

---
