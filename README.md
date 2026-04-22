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

## Project Status

| Component | Status | Notes |
|-----------|--------|-------|
| Core Framework (CLI, Agent Core, Integration) | Complete | Stable interactive workflow and orchestration |
| Exploit Modules | Active | BYOVD, EDR-Freeze, and Crystal Palace implemented; additional techniques planned |
| ML Framework | Complete | Adaptive recommendation and analysis pipeline integrated |

Overall status: Operational core with extensible module architecture.

---

## Architecture

The framework follows a layered modular architecture.

![Architecture Diagram](https://github.com/user-attachments/assets/8706fbd0-6f73-4fb6-afdf-3c3273fd7bea)

---

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

Status: Implemented and tested.

- CLI: interactive operation, command routing, technique orchestration
- Agent Core: execution orchestration, telemetry handling, artifact cleanup
- Integration: EDR connectors, snapshot manager, environment helpers

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
