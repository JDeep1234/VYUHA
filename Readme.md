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
│   │   └── exploit_manager.hpp # Exploit techniques [BIPIN]
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

| Technique ID | Name | Tactic | Owner |
|--------------|------|--------|-------|
| T1055 | Process Injection | Defense Evasion | Bipin |
| T1055.012 | Process Hollowing | Defense Evasion | Bipin |
| T1218.002 | Control Panel | Defense Evasion | Bipin |
| T1218.005 | Mshta | Defense Evasion | Bipin |
| T1574.002 | DLL Side-Loading | Persistence | Bipin |
| T1106 | Native API (Syscalls) | Execution | Bipin |
| T1562.001 | Disable Security Tools | Defense Evasion | Bipin |

---

## 🔧 Module Details

### 🟢 Jdeep's Modules (CLI, Agent Core, Integration)

**CLI Tool (`src/cli/`)**
- Command parsing with Typer-style interface
- Banner and help display
- Option handling

**Agent Core (`src/agent_core/`)**
- `Orchestrator`: Execution state machine
- `OutputHandler`: JSON, CSV, HTML, STIX export
- `Cleaner`: Artifact removal (files, registry, services)
- `TelemetryMonitor`: EDR event monitoring

**Integration (`src/integration/`)**
- `EDRConnector`: CrowdStrike, Defender, Carbon Black APIs
- `SnapshotManager`: Hyper-V, VirtualBox, VMware support
- `CleanModule`: System backup and restore

### 🟠 Bipin's Module (Exploit Scripts)

**Location:** `src/exploits/`

**Techniques to Implement:**
- `CPLAttack`: Control Panel execution via rundll32
- `DLLSideload`: DLL hijacking (MS Teams, etc.)
- `EXEInjection`: EarlyBird APC injection
- `HTAAttack`: Mshta.exe execution
- `EDRRedir`: Minifilter bind attack
- `DirectSyscalls`: ETW/callback bypass

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

### 🟣 Karthik's Module (ML Framework)

**Location:** `src/ml_framework/`

**Components to Implement:**
- `DetectionAnalyzer`: Pattern recognition, ML detection
- `EvasionScorer`: Success rate calculation
- `EventCorrelator`: MITRE ATT&CK mapping

**How to Add ML Model:**
```cpp
// In src/ml_framework/your_analyzer.cpp
class YourAnalyzer : public BaseAnalyzer {
public:
    void analyze(const void* executionResult) override {
        // Your ML analysis
    }
    
    std::string getReport() const override {
        // Return analysis results
    }
};
```

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



