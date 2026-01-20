# 🛡️ EDR Adaptive Framework 


> **Advanced APT Simulation & EDR Evasion Assessment Platform**

A comprehensive framework for testing and evaluating Endpoint Detection and Response (EDR) solutions through simulated Advanced Persistent Threat (APT) techniques, mapped to the MITRE ATT&CK framework.

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

<img width="1596" height="1072" alt="image" src="https://github.com/user-attachments/assets/8706fbd0-6f73-4fb6-afdf-3c3273fd7bea" />

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


## 📚 References

- [EDR-Redir](https://github.com/TwoSevenOneT/EDR-Redir) - Bind Filter Attack PoC
- [MDPI Paper](https://www.mdpi.com/2624-800X/1/3/21) - EDR Bypass Research
- [MITRE ATT&CK](https://attack.mitre.org/) - Adversarial Tactics & Techniques
- [LOLBAS Project](https://lolbas-project.github.io/) - Living Off The Land Binaries

