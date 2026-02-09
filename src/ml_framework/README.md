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
├── DESIGN.md              # Comprehensive design documentation (READ THIS FIRST!)
├── README.md              # This file
├── requirements.txt       # Python dependencies
│
├── python/                # Python ML implementation (create this)
│   ├── strategy_selector.py
│   ├── behavior_analyzer.py
│   ├── explainable_ai.py
│   └── adaptive_learner.py
│
├── models/                # Trained models (create this)
│   └── .gitkeep
│
├── data/                  # Training data (create this)
│   └── .gitkeep
│
├── ml_bridge.cpp          # C++ ↔ Python interface (to be created)
├── ml_engine.cpp          # Existing C++ implementation
└── ml_engine.hpp          # Existing header
```

---

## 🎯 Your Implementation Tasks

### Phase 1: Core RL Agent (Week 1-2)
- [ ] Implement `strategy_selector.py` with DQN
- [ ] Define state space (30 features)
- [ ] Define action space (8 techniques)
- [ ] Define reward function

### Phase 2: Behavior Analysis (Week 3)
- [ ] Implement `behavior_analyzer.py`
- [ ] EDR response feature extraction
- [ ] K-means clustering for EDR grouping

### Phase 3: Explainable AI (Week 4)
- [ ] Implement `explainable_ai.py`
- [ ] SHAP integration
- [ ] LIME integration
- [ ] Generate human-readable explanations

### Phase 4: Adaptive Learning (Week 5)
- [ ] Implement `adaptive_learner.py`
- [ ] Online learning updates
- [ ] Transfer learning between EDRs
- [ ] Failure pattern analysis

### Phase 5: C++ Integration (Week 6)
- [ ] Create `ml_bridge.cpp` with pybind11
- [ ] Integrate with existing `ml_engine.cpp`
- [ ] Test end-to-end pipeline

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
| EDR Termination Success Rate | >60% | TBD |
| Avg Techniques/Episode | <4 | TBD |
| Prediction Accuracy (Top-3) | >70% | TBD |
| Training Episodes to Convergence | <200 | TBD |

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

### "Segfault in pybind11"
Check data type conversions in ml_bridge.cpp

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
