# 🧠 ML Framework - Quick Start

**Author:** Karthik  
**Purpose:** Reinforcement Learning-driven EDR termination strategy selection

---

## 📋 Overview

This module uses **Reinforcement Learning** to intelligently select which EDR termination technique to attempt based on:
- Current system state (Windows version, PPL status, etc.)
- EDR product & version
- Historical success/failure patterns

Key Features:
- **Deep Q-Network (DQN)** for action selection
- **Explainable AI (SHAP/LIME)** for failure analysis
- **Behavior clustering** to identify EDR defense patterns
- **Transfer learning** between similar EDR products

---

## 🚀 Quick Setup

### 1. Install Python Dependencies

```bash
cd src/ml_framework
pip install -r requirements.txt
```

### 2. Verify Installation

```bash
python -c "import torch; import shap; import pybind11; print('✓ All dependencies installed')"
```

---

## 📂 File Structure

```
ml_framework/
├── DESIGN.md                   # Comprehensive design documentation
├── ROADMAP.md                  # Implementation roadmap
├── README.md                   # This file
├── requirements.txt            # Python dependencies
│
├── python/                     # Python ML implementation ✅ Complete
│   ├── strategy_selector.py    # ✅ DQN agent (state=30, actions=8)
│   ├── behavior_analyzer.py    # ✅ K-means EDR clustering
│   ├── explainable_ai.py       # ✅ SHAP/gradient explainability
│   ├── adaptive_learner.py     # ✅ Online/transfer learning
│   ├── ml_server.py            # ✅ stdio JSON bridge server (10 cmds)
│   ├── utils.py                # ✅ Shared constants & reward helpers
│   └── train_agent.py          # ✅ Standalone training CLI
│
├── models/                     # Trained models (populated at runtime)
│
├── data/                       # Training data ✅ Seeded
│   ├── edr_profiles.json       # ✅ 6 EDR seed profiles for clustering
│   └── execution_logs.csv      # ✅ Seed execution log data
│
├── detection_analyzer.cpp      # ✅ 22-keyword weighted alert detection
├── evasion_scorer.cpp          # ✅ Rolling per-technique stealth scoring
├── event_correlator.cpp        # ✅ MITRE ATT&CK correlation (10 entries)
├── ml_bridge.cpp               # ✅ C++ ↔ Python subprocess bridge (Win32)
├── ml_engine.cpp               # ✅ Full analyze/recommend/report/save/load
└── ml_engine.hpp               # ✅ Clean class declarations
```

---

## ✅ Implementation Status — Complete

### Phase 1: Core RL Agent
- [x] `strategy_selector.py` — DQN with policy/target networks, epsilon-greedy, experience replay
- [x] State space: 30 features (EDR one-hot, runtime state, Windows version, system config, last action)
- [x] Action space: 8 techniques (BYOVD RTCore/DBUtil, PPL Bypass, Handle Dup, Minifilter, Callback, Syscall, Wait)
- [x] Reward function: +100 terminated, +75 silenced, +50 partial, -50 detected, -75 blocked, -100 crash; stealth bonus/burn penalty

### Phase 2: Behavior Analysis
- [x] `behavior_analyzer.py` — K-means n=4, 11-feature EDR response extraction
- [x] EDR response feature extraction via `analyze_response()`
- [x] K-means clustering with `cluster_edrs()` and persistence to `data/edr_observations.json`

### Phase 3: Explainable AI
- [x] `explainable_ai.py` — SHAP KernelExplainer on DQN predict wrapper
- [x] Gradient-based fallback when shap not installed
- [x] `feature_importance_global()`, `compare_techniques()`, `_build_narrative()`

### Phase 4: Adaptive Learning
- [x] `adaptive_learner.py` — `online_update()` calls `agent.remember()` + `agent.replay()`
- [x] `transfer_learning()` with cosine similarity weight check
- [x] `analyze_failure_patterns()` blacklists (edr, technique) pairs; persists to `data/execution_history.json`

### Phase 5: C++ Integration
- [x] `ml_bridge.cpp` — Windows `CreateProcess` subprocess (JSON stdio, no pybind11)
- [x] `ml_engine.cpp` — full three-analyzer loop wired into `AgentCore::runCampaign()`
- [x] `ml_server.py` — 10-command stdio server bridging C++ ↔ Python
- [x] End-to-end pipeline: CLI → AgentCore → [DQN → Bipin exploit → analyze → train]

---

## 📖 Complete Documentation

**For detailed design, RL formulation, code examples, and architecture:**

👉 **Read [`DESIGN.md`](DESIGN.md)** 👈

This contains:
- Full RL formulation (state/action/reward)
- Complete Python code for all 4 components
- C++ integration examples
- Evaluation metrics
- Usage examples

---

## 🧪 Testing

### Test RL Agent (Standalone)

```python
# In python/
python strategy_selector.py --test
```

### Test with C++ Framework

```bash
# Build entire project
cd ../../build
cmake --build . --config Release

# Run with ML enabled
./bin/edr_framework.exe run --ml-enabled --edr defender
```

---

## 📊 Model Training

### Train on Single EDR

```bash
python python/train_agent.py --edr defender --episodes 500 --save models/dqn_defender.pth
```

### Transfer Learning

```bash
# Use Defender model as starting point for CrowdStrike
python python/train_agent.py --edr crowdstrike --transfer models/dqn_defender.pth --episodes 200
```

---

## 🔍 Generate Analysis Reports

### XAI Failure Analysis

```bash
python python/explain_failures.py \
  --logs data/execution_logs.csv \
  --output reports/failure_analysis.html
```

### EDR Clustering Report

```bash
python python/behavior_analyzer.py \
  --data data/edr_behaviors.csv \
  --clusters 4 \
  --output reports/edr_clusters.json
```

---

## 🎓 Learning Resources

### Reinforcement Learning
- [Spinning Up in Deep RL](https://spinningup.openai.com/)
- [DQN Paper](https://arxiv.org/abs/1312.5602) - Mnih et al., 2013

### Explainable AI
- [SHAP Documentation](https://shap.readthedocs.io/)
- [LIME Paper](https://arxiv.org/abs/1602.04938)

### Python-C++ Integration
- [pybind11 Tutorial](https://pybind11.readthedocs.io/)

---

## 🐛 Debugging

### Python Side

```bash
# Enable verbose logging
export ML_DEBUG=1
python python/strategy_selector.py --verbose
```

### C++ Side

```cpp
// In ml_engine.cpp
#define ML_DEBUG 1
```

---

## 📈 Performance Benchmarks

Target metrics for successful implementation:

| Metric | Target | Current |
|--------|--------|---------|
| EDR Termination Success Rate | >60% | Tracked live via `EvasionScorer` |
| Avg Techniques/Episode | <4 | Tracked live via campaign results |
| Prediction Accuracy (Top-3) | >70% | Logged per episode in `execution_logs.csv` |
| Training Episodes to Convergence | <200 | Run `train_agent.py --episodes 500 --verbose` |

---

## 💡 Tips

1. **Start Simple**: Implement basic DQN first, optimize later
2. **Log Everything**: Capture all state/action/reward for analysis
3. **Visualize**: Plot reward curves, action distributions
4. **Test Incrementally**: Verify each component before integration
5. **Read DESIGN.md**: All the details are there!

---

## ⚠️ Common Issues

### "Python not found"
Install Python 3.8+ and add to PATH

### "Module 'torch' not found"
Run `pip install -r requirements.txt`

### "Python subprocess not responding"
- Ensure Python is on PATH: `python --version`
- Check `ml_server.log` (written next to the binary) for Python-side errors
- The bridge uses subprocess JSON stdio — no pybind11 required

### "Model not learning"
- Verify reward function is correct
- Check if replay buffer has enough samples
- Tune hyperparameters (learning_rate, epsilon)

---

## 📧 Need Help?

- Read [`DESIGN.md`](DESIGN.md) first
- Check existing `ml_engine.cpp` for C++ integration points
- Ask team members: Jdeep (framework), Bipin (techniques)

---

**Good luck with your ML implementation! 🚀**
