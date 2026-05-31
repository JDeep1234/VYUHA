<img width="1100" height="400" alt="VYUHA Architecture" src="https://github.com/user-attachments/assets/8706fbd0-6f73-4fb6-afdf-3c3273fd7bea" />

<p align="center">
  <a href="#what-is-vyuha">What is VYUHA?</a> •
  <a href="#the-ml-loop-that-learns-every-product">ML Loop</a> •
  <a href="#what-vyuha-exposes-today">Results</a> •
  <a href="#attack-flow">Flow</a> •
  <a href="#architecture">Architecture</a> •
  <a href="#quick-start">Quick Start</a> •
  <a href="#module-reference">Modules</a>
</p>

<p align="center">
  <img alt="C++17" src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus" />
  <img alt="MSVC v143" src="https://img.shields.io/badge/MSVC-v143-5C2D91?logo=visualstudio" />
  <img alt="CMake" src="https://img.shields.io/badge/CMake-3.16+-064F8C?logo=cmake" />
  <img alt="Python" src="https://img.shields.io/badge/Python-DQN%20%2B%20SHAP-3776AB?logo=python&logoColor=white" />
  <img alt="MITRE ATT&CK" src="https://img.shields.io/badge/MITRE-ATT%26CK%20Mapped-e6292f" />
  <img alt="Windows 11" src="https://img.shields.io/badge/Windows-11%20x64-0078D4?logo=windows11&logoColor=white" />
  <img alt="ACM CCS 2026" src="https://img.shields.io/badge/ACM%20CCS-2026-222222" />
</p>

# VYUHA

> **Four cross-layer exploit techniques + a DQN agent that learns which sequence breaks each EDR — with transfer learning that carries knowledge across products and SHAP explanations that tell you exactly why.**

## The Problem

A security team runs a BYOVD test against their EDR. It blocks the driver load. They check the box and move on. But they never test direct syscall invocation, which slips past the same product in two out of three attempts with a 33% detection rate. They never test reflective loading behind six evasion layers. They never learn that their EDR's user-mode hook coverage has a blind spot that an attacker with the right toolchain will find.

The usual fix is manual red-team engagements: pick techniques one at a time, run them in sequence, write a report. But static test plans understate real-world risk. An EDR that blocks kernel-level termination may be blind to syscall-boundary evasion. A product that catches process suspension may miss reflective loading. Testing techniques individually, manually, and without adaptation is neither repeatable nor scalable.

VYUHA solves this by treating EDR evaluation as a reinforcement learning problem. The system observes how each product responds — which techniques get blocked, which generate alerts without blocking, which go undetected — and adapts its attack strategy in real time. A policy trained against one product transfers to the next, cutting convergence time by 4×. No manual sequencing, no static playbooks. Just an adversary that learns.

## What Is VYUHA?

VYUHA is built for blue teams and EDR vendors who need to know where their detection pipeline actually fails — not where they assume it works.

| From one evaluation target | VYUHA produces | Why it matters |
| --- | --- | --- |
| Kernel, user-mode, and syscall-boundary techniques | Four ATT&CK-mapped exploits executed against the target EDR | Every defensive layer gets tested, not just the one you remembered |
| Live telemetry from process state, event logs, and EDR responses | A 26-dimensional state vector fed into a DQN agent | The next technique is chosen by observed behavior, not a static list |
| Per-technique success rates, detection rates, and time-to-detection | Structured results exportable as JSON, CSV, HTML, or STIX 2.1 | Gaps become engineering priorities with reproducible evidence |
| Accumulated behavioral profiles across products | K-means clustering, SHAP attribution, and transfer learning | Knowledge compounds — the second product evaluation starts where the first left off |

## The ML Loop That Learns Every Product

Every EDR evaluation is a closed feedback loop. The DQN agent picks a technique, observes the outcome, updates its Q-values, and picks again. After ~80 episodes, it converges on a product-specific policy. When a new EDR is introduced, cosine similarity over an 11-dimensional behavioral vector identifies the closest match, and the trained policy's weights transfer directly — cutting convergence from ~150 episodes to ~37.

SHAP attribution runs after every failure, identifying which system properties (PPL protection, driver signature enforcement, kernel callbacks) drove the detection. K-means clustering ($k{=}4$) groups products into tiers — Enterprise Grade, Moderate Hybrid, User-Mode Focused, Basic Detection — so defenders can benchmark their deployed EDR against the landscape.

## What VYUHA Exposes Today

Evaluated against five EDR products. No product detected all four techniques.

| | **BYOVD (T1068)** | **EDR-Freeze (T1562.001)** | **Crystal Palace (T1055.001)** | **SysWhispers4 (T1106)** |
|---|---|---|---|---|
| **Defender** | 67% det · 2480ms | 33% det · 3100ms | 33% det · 2895ms | 33% det · 3404ms |
| **SentinelOne** | 100% det · 1820ms | 100% det · 1450ms | 67% det · 2553ms | 33% det · 3910ms |
| **OpenEDR** | 0% det · — | 33% det · 3200ms | 0% det · — | 0% det · — |
| **Huntress** | 100% det · 2150ms | 100% det · 1870ms | 67% det · 3159ms | 67% det · 2228ms |
| **Trend Micro** | 67% det · 2880ms | 33% det · 2640ms | 67% det · 2330ms | 0% det · — |

**Key findings:**
- **SysWhispers4** has the lowest mean detection rate across all products: **27%**
- **SentinelOne & Huntress** block kernel-level and process-suspension attacks entirely — but Crystal Palace and SysWhispers4 still get through partially
- **OpenEDR** failed to detect three of four techniques entirely (8.3% overall mean detection)
- **Transfer learning** delivers **3.8–4.1× speedup** in convergence across all EDR pairs

## Attack Flow

```
┌─────────────────────┐
│   Operator Interface │  Campaign spec + technique selection
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│     Agent Core       │  Execution orchestrator + telemetry monitor
│  ┌───────────────┐   │
│  │ Campaign Runner│──►│──── Executes recommended technique
│  └───────────────┘   │
└──────────┬──────────┘
           │ state observation (26-dim vector)
           ▼
┌─────────────────────┐
│  ML Analysis Engine  │  DQN action selection + reward computation
│  ┌────┐ ┌────┐      │
│  │DQN │ │SHAP│      │  + K-means clustering + transfer learning
│  └────┘ └────┘      │
└──────────┬──────────┘
           │ action recommendation
           ▼
┌─────────────────────┐
│  Exploit Deployment  │  Four modules, uniform interface
│  ┌──────┐ ┌───────┐ │
│  │BYOVD │ │Freeze │ │  T1068 · T1562.001 · T1055.001 · T1106
│  └──────┘ └───────┘ │
│  ┌───────┐ ┌──────┐ │
│  │Crystal│ │SysW4 │ │
│  └───────┘ └──────┘ │
└─────────────────────┘
           │
           ▼
    Feedback → Agent Core → ML Engine → next action (closed loop)
```

## Architecture

**Language:** C++17 · MSVC v143 · Windows 11 x86_64
**ML bridge:** C++ ↔ Python via subprocess JSON protocol over anonymous pipes
**Exploit coverage:** 70+ EDR process signatures across 17 vendor families

| Subsystem | Role |
|---|---|
| **Operator Interface** | CLI with numbered menus, batch mode, multi-format export (JSON/CSV/HTML/STIX 2.1) |
| **Agent Core** | Execution orchestrator (8-state lifecycle), campaign runner, telemetry monitor (2 background threads), artifact cleaner |
| **Exploit Deployment** | Registry of four modules with uniform metadata/execute/cleanup interface, keyed by MITRE ATT&CK ID |
| **ML Analysis Engine** | 11-command JSON protocol: action selection, training, target sync, persistence, clustering, SHAP, transfer learning |
| **Telemetry Monitor** | Process snapshot polling + Windows Event Log delta reads via mutex-protected buffering |

## Project Structure

```
VYUHA/
├── CMakeLists.txt
├── config/
│   └── config.yaml
├── include/
│   ├── cli/cli.hpp
│   ├── agent_core/agent.hpp
│   ├── integration/integration_manager.hpp
│   ├── exploits/
│   │   ├── exploit_manager.hpp
│   │   ├── byovd_vulndriver.hpp
│   │   ├── edr_freeze.hpp
│   │   └── crystal_palace_loader.hpp
│   └── ml_framework/ml_engine.hpp
├── src/
│   ├── main.cpp
│   ├── cli/cli.cpp
│   ├── agent_core/
│   │   ├── agent.cpp
│   │   ├── cleaner.cpp
│   │   ├── orchestrator.cpp
│   │   ├── output_handler.cpp
│   │   └── telemetry.cpp
│   ├── exploits/
│   │   ├── exploit_manager.cpp
│   │   ├── byovd_vulndriver.cpp
│   │   ├── edr_freeze.cpp
│   │   ├── crystal_palace_loader.cpp
│   │   └── syswhispers4_syscall.cpp
│   ├── integration/
│   │   ├── integration_manager.cpp
│   │   ├── edr_connector.cpp
│   │   ├── snapshot_manager.cpp
│   │   └── clean_module.cpp
│   └── ml_framework/
│       ├── ml_engine.cpp
│       ├── ml_bridge.cpp
│       ├── detection_analyzer.cpp
│       ├── evasion_scorer.cpp
│       ├── event_correlator.cpp
│       └── python/
│           ├── ml_server.py
│           ├── strategy_selector.py
│           ├── adaptive_learner.py
│           ├── behavior_analyzer.py
│           ├── explainable_ai.py
│           ├── train_agent.py
│           └── utils.py
└── contexts/
    └── *.md                    # Per-module documentation
```

## Quick Start

### Prerequisites

- Windows 11 x86_64 (build 22631+)
- CMake 3.16+
- MSVC v143 (Visual Studio 2022)
- Python 3.10+ with `torch`, `numpy`, `shap`, `scikit-learn`
- Administrator privileges (kernel driver operations require `SeLoadDriverPrivilege`)

### Build

```bash
git clone git@github.com:JDeep1234/VYUHA.git
cd VYUHA

mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Run

```bash
# Interactive mode
./bin/edr_framework.exe

# Single technique
./bin/edr_framework.exe run -t T1068 --verbose

# Full campaign
./bin/edr_framework.exe campaign -c campaign.yaml -o json

# List techniques
./bin/edr_framework.exe list

# Check EDR status
./bin/edr_framework.exe status
```

## Module Reference

| ID | Module | MITRE | Layer | What It Does |
|---|---|---|---|---|
| 0 | **BYOVD VulnDriver** | T1068 | Kernel | Loads signed vulnerable driver (`wsftprm.sys`, CVE-2023-52271), sends IOCTL `0x22201C` to call `ZwTerminateProcess` from kernel context — bypasses PPL |
| 1 | **EDR-Freeze** | T1562.001 | User | Exploits `WerFaultSecure.exe` race condition to suspend EDR threads via dump-lock deadlock — no driver, no child process, no file write |
| 2 | **Crystal Palace** | T1055.001 | User | Six-layer reflective loader: XOR decrypt → module overload → CFG registration → `NtContinue` context transfer → Draugr call stack spoofing → sleep-time memory masking |
| 3 | **SysWhispers4** | T1106 | Syscall | Resolves NT SSNs via six strategies (FreshyCalls, Hell's/Halo's/Tartarus'/RecycledGate, FromDisk), supports direct and indirect invocation with ETW/AMSI bypass |

## DQN Configuration

| Parameter | Value |
|---|---|
| State space | 26 dimensions (EDR identity, runtime state, Windows version, system config, action history) |
| Action space | 4 discrete actions (one per exploit module) |
| Network | 26 → 128 → 64 → 4 with ReLU + 20% dropout |
| Optimizer | Adam, lr = 10⁻³ |
| Replay buffer | 10,000 transitions, minibatch size 32 |
| Target network sync | Every 10 episodes |
| Exploration | ε-greedy: ε₀ = 1.0, ε_min = 0.01, decay = 0.995 |
| Discount factor | γ = 0.95 |
| Transfer threshold | Cosine similarity ≥ 0.7 |

## Tested EDR Products

| EDR | Tier | Primary Detection | Overall Mean Detection |
|---|---|---|---|
| Microsoft Defender | Moderate | Kernel callbacks, ETWTi, cloud ML | 41.5% |
| SentinelOne | Enterprise | Behavioral AI, PPL, kernel callbacks | 75.0% |
| OpenEDR | Basic | Open-source, process event alerts | 8.3% |
| Huntress | Moderate | Cloud-managed, process monitoring | 83.5% |
| Trend Micro Apex One | Moderate | Kernel callbacks, ETW, AMSI, hooks | 41.8% |

## Responsible Use

VYUHA is an adversarial evaluation framework for authorized security testing only. Its primary purpose is to help **blue-team defenders and EDR vendors** discover detection blind spots with reproducible evidence before real adversaries do. Use only in controlled lab environments with proper authorization.

---

<p align="center">
  <sub>ACM CCS 2026 · Cross-Layer EDR Kill-Chain Evasion via Deep Reinforcement Learning-Guided Adversarial Orchestration</sub>
</p>
