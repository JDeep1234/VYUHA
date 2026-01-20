# 🛡️ EDR Adaptive Framework v2.0

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Python 3.9+](https://img.shields.io/badge/python-3.9+-blue.svg)](https://www.python.org/downloads/)
[![MITRE ATT&CK](https://img.shields.io/badge/MITRE-ATT%26CK-red.svg)](https://attack.mitre.org/)

> **Advanced APT Simulation & EDR Evasion Assessment Platform**

A comprehensive framework for testing and evaluating Endpoint Detection and Response (EDR) solutions through simulated Advanced Persistent Threat (APT) techniques, mapped to the MITRE ATT&CK framework.

---

## 📋 Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Features](#features)
- [Modules](#modules)
- [Installation](#installation)
- [Usage](#usage)
- [MITRE ATT&CK Coverage](#mitre-attck-coverage)
- [Supported EDR/AV Systems](#supported-edrav-systems)
- [Team](#team)
- [References](#references)
- [Disclaimer](#disclaimer)

---

## 🎯 Overview

The EDR Adaptive Framework is designed to help security teams:

- **Assess EDR Effectiveness**: Test how well your EDR solution detects various attack techniques
- **Simulate APT Campaigns**: Execute realistic attack chains mapped to the Cyber Kill Chain
- **Identify Detection Gaps**: Discover blind spots in your security posture
- **Generate Compliance Reports**: Produce detailed reports with MITRE ATT&CK mappings

---

## 🏗️ Architecture

The framework follows a modular architecture with distinct layers for flexibility and extensibility:

<a href="docs/architecture.drawio">
  <img src="https://app.diagrams.net/img/drawio.svg" alt="View Architecture Diagram" width="200"/>
</a>

**[📐 View Full Architecture Diagram (draw.io)](docs/architecture.drawio)**

> 📌 **To view the architecture diagram**:
> - Click the link above to download and open in [draw.io](https://app.diagrams.net/)
> - Use the VS Code [Draw.io Integration](https://marketplace.visualstudio.com/items?itemName=hediet.vscode-drawio) extension
> - Or import directly at [app.diagrams.net](https://app.diagrams.net/)

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        EXTERNAL INTERFACES                              │
│  Web Dashboard │ REST API │ CLI Tool │ Scheduler │ Notifications        │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                        CORE FRAMEWORK ENGINE                            │
│  ┌──────────────┐  ┌────────────────┐  ┌─────────────────┐             │
│  │  Agent Core  │  │ Exploit Module │  │  ML Framework   │             │
│  │  • Orchestr. │  │ • ATT&CK Tech. │  │  • Detection    │             │
│  │  • Config    │  │ • Payloads     │  │  • Scoring      │             │
│  │  • Plugins   │  │ • Evasion      │  │  • Correlation  │             │
│  └──────────────┘  └────────────────┘  └─────────────────┘             │
│  ┌──────────────┐  ┌────────────────┐  ┌─────────────────┐             │
│  │  Security    │  │ Anti-Analysis  │  │ Report Generator│             │
│  │  • Encrypt   │  │ • Sandbox Det. │  │  • PDF/HTML     │             │
│  │  • Auth/RBAC │  │ • Anti-Debug   │  │  • Dashboards   │             │
│  │  • Audit     │  │ • Hook Detect  │  │  • STIX Export  │             │
│  └──────────────┘  └────────────────┘  └─────────────────┘             │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                       INFRASTRUCTURE LAYER                              │
│  Integration & Snapshot │ Network Layer │ Forensics Module              │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## ✨ Features

### 🔧 Core Capabilities

| Feature | Description |
|---------|-------------|
| **Modular Design** | Plugin-based architecture for easy extension |
| **MITRE ATT&CK Mapping** | All techniques mapped to ATT&CK framework |
| **Automated Testing** | Scheduled and batch execution support |
| **Real-time Monitoring** | Live telemetry and EDR event capture |
| **Comprehensive Reporting** | PDF, HTML, JSON, and STIX export formats |

### 🔒 Security Features

- **AES-256/RSA Encryption** for payloads and communications
- **JWT/OAuth2 Authentication** with RBAC
- **Immutable Audit Trail** for compliance
- **Artifact Cleanup** for safe testing

---

## 📦 Modules

### 🟢 Agent Core
- **Orchestrator**: Execution flow and state machine management
- **Config Manager**: YAML/JSON configuration with environment variables
- **Plugin Loader**: Dynamic import and hot reload capabilities
- **Output Handler**: Multi-format result streaming
- **Telemetry**: EDR monitoring and event capture

### 🟠 Exploit Scripts Module
Attack techniques categorized by MITRE ATT&CK tactics:

| Tactic | Techniques |
|--------|------------|
| **Initial Access** | Phishing Payloads, Drive-by Download, Supply Chain |
| **Execution** | CPL (rundll32), HTA (mshta), PowerShell, WMI/COM |
| **Persistence** | Registry Keys, Scheduled Tasks, Services, WMI Subscriptions |
| **Privilege Escalation** | Token Manipulation, UAC Bypass, CVE Exploits |
| **Defense Evasion** | DLL Sideload, Process Injection, EDR-Redir, Direct Syscalls |
| **Credential Access** | LSASS Dump, Kerberoasting, SAM Extract |
| **Lateral Movement** | PsExec/WMI, RDP Hijack, Pass-the-Hash |
| **C2** | HTTP/S Beacon, DNS Tunnel, SMB Pipe |

### 🟣 ML Framework
- **Detection Engine**: Pattern recognition and anomaly detection
- **Evasion Scorer**: Success rate calculation and risk analysis
- **Event Correlator**: ATT&CK mapping and timeline building
- **Predictive Model**: EDR behavior prediction and gap analysis

### 🔴 Anti-Analysis Module
- **Sandbox Detection**: VM detection and timing checks
- **Anti-Debug**: Debugger detection and Int3 traps
- **Hook Detector**: API hooks and ETW patch detection
- **Environment Fingerprinting**: User activity and system profiling

### 🧬 Payload Generator
- **Shellcode Generation**: Donut, ScareCrow, Custom ASM
- **Obfuscation**: String encryption, control flow obfuscation
- **Packer/Crypter**: UPX, custom packers, code signing

---

## 🚀 Installation

```bash
# Clone the repository
git clone https://github.com/JDeep1234/EDR-Adaptive-Framework.git
cd EDR-Adaptive-Framework

# Create virtual environment
python -m venv .venv
source .venv/bin/activate  # Linux/Mac
# or
.\.venv\Scripts\Activate.ps1  # Windows PowerShell

# Install dependencies
pip install -r requirements.txt

# Configure the framework
cp config/config.example.yaml config/config.yaml
# Edit config.yaml with your settings
```

---

## 💻 Usage

### CLI Interface

```bash
# Run a specific technique
python -m edr_framework run --technique T1055 --target localhost

# Execute full attack chain
python -m edr_framework campaign --config campaigns/apt29.yaml

# Generate report
python -m edr_framework report --format pdf --output results/

# List available techniques
python -m edr_framework list --tactics all
```

### API Usage

```python
from edr_framework import Agent, TechniqueLoader

# Initialize agent
agent = Agent(config="config/config.yaml")

# Load and execute technique
technique = TechniqueLoader.get("T1055.001")  # Process Injection
result = agent.execute(technique, target="explorer.exe")

# Check results
print(f"Detected: {result.detected}")
print(f"Evasion Score: {result.evasion_score}")
```

---

## 🗺️ MITRE ATT&CK Coverage

The framework covers techniques across the Cyber Kill Chain:

```
1. Reconnaissance  →  2. Weaponization  →  3. Delivery  →  4. Exploitation
       ↓                                                          ↓
5. Installation  ←  6. Command & Control  ←  7. Actions on Objectives
```

### Evasion Techniques Library

| Technique | Description |
|-----------|-------------|
| Direct Syscalls | Bypass user-mode hooks via direct NT syscalls |
| PPID Spoofing | Fake parent process to evade behavioral detection |
| DLL Sideloading | Abuse legitimate applications for DLL loading |
| Process Hollowing | Replace legitimate process memory with malicious code |
| ETW Patching | Disable Event Tracing for Windows |
| LOLBAS | Living Off The Land Binaries and Scripts |
| Callback Patching | Remove EDR kernel callbacks |
| Module Stomping | Overwrite legitimate module memory |
| Thread Hijacking | Execute code in existing thread context |
| AMSI Bypass | Disable Antimalware Scan Interface |

---

## 🎯 Supported EDR/AV Systems

The framework has been tested against:

| Vendor | Product | Status |
|--------|---------|--------|
| CrowdStrike | Falcon | ✅ Tested |
| Microsoft | Defender ATP | ✅ Tested |
| VMware | Carbon Black | ✅ Tested |
| Sophos | Intercept X | ✅ Tested |
| ESET | Endpoint Security | ✅ Tested |
| Kaspersky | Endpoint Security | ✅ Tested |

---

## 👥 Team

| Member | Responsibilities |
|--------|-----------------|
| **Jdeep** | Core Framework, Integration & Snapshot, Infrastructure |
| **Member 1** | ML Framework, Detection & Scoring |
| **Member 2/3** | Exploit Scripts Module |

---

## 📚 References

- [EDR-Redir](https://github.com/TwoSevenOneT/EDR-Redir) - Bind Filter Attack PoC
- [MDPI Paper](https://www.mdpi.com/2624-800X/1/3/21) - EDR Bypass Research
- [MITRE ATT&CK](https://attack.mitre.org/) - Adversarial Tactics & Techniques
- [LOLBAS Project](https://lolbas-project.github.io/) - Living Off The Land Binaries

