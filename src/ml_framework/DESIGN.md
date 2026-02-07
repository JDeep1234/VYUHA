# 🧠 ML Framework - Design Documentation

**Author:** Karthik  
**Project:** EDR Adaptive Framework - ML-Driven Attack Strategy Selection  
**Last Updated:** February 7, 2026

---

## 🎯 Overview

This ML framework implements an **Adaptive EDR Evasion System** using Reinforcement Learning to intelligently select and optimize attack strategies against endpoint security products. The system learns from each execution attempt to improve success rates while providing explainable insights into why attacks succeed or fail.

### Core Philosophy

> **From Static to Adaptive:** Instead of blindly trying all techniques, use ML to learn which attacks work best against specific EDR products and system configurations.

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ML Framework (Python)                     │
├─────────────────────────────────────────────────────────────┤
│  ┌────────────────┐  ┌────────────────┐  ┌──────────────┐  │
│  │ StrategySelector│  │BehaviorAnalyzer│  │ExplainableAI │  │
│  │   (RL Agent)   │  │  (Clustering)  │  │ (SHAP/LIME)  │  │
│  └────────────────┘  └────────────────┘  └──────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │          AdaptiveLearner (Online Training)           │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ↕ (pybind11)
┌─────────────────────────────────────────────────────────────┐
│              C++ Framework (ml_engine.cpp)                   │
├─────────────────────────────────────────────────────────────┤
│  • State collection      • Reward calculation               │
│  • Python bridge         • Result logging                   │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│           EDR Termination Execution (Bipin's Module)        │
└─────────────────────────────────────────────────────────────┘
```

---

## 📦 Component 1: StrategySelector (RL Agent)

### Purpose
Select the optimal EDR termination technique based on current system state and historical success patterns.

### Reinforcement Learning Formulation

#### **Agent**
Deep Q-Network (DQN) or Proximal Policy Optimization (PPO) model

#### **State Space (s)** - Dimensions: 25-30 features

```python
state = {
    # EDR Information (8 features)
    'edr_product': one_hot_encoded(5),      # Defender, CrowdStrike, etc.
    'edr_version': float,                    # Normalized version number
    'edr_process_running': bool,             # Is EDR service active?
    'edr_driver_loaded': bool,               # Is kernel driver active?
    
    # System Configuration (10 features)
    'windows_version': one_hot_encoded(4),   # Win10, Win11, Server
    'windows_build': int,                     # Build number (normalized)
    'integrity_level': one_hot_encoded(3),   # Medium, High, System
    'ppl_protection': bool,                   # PPL/PPL-Light active?
    'driver_sig_enforcement': bool,           # DSE status
    'secure_boot': bool,                      # UEFI Secure Boot
    'virtualization_enabled': bool,           # Hyper-V/VBS
    'antimalware_light': bool,                # ELAM status
    
    # Previous Attempts (7 features)
    'last_action': one_hot_encoded(8),       # Previous technique tried
    'last_action_result': one_hot_encoded(4), # Success/Blocked/Failed/Crash
    'consecutive_failures': int,              # Failure streak count
    'techniques_tried': list[bool],           # Which techniques already attempted
    'time_since_last_action': float,          # Seconds elapsed
}
```

#### **Action Space (a)** - 8 Discrete Actions

```python
actions = {
    0: "BYOVD_RTCore",          # Load RTCore64.sys vulnerable driver
    1: "BYOVD_DBUtil",          # Load DBUtil driver
    2: "PPL_Bypass",            # Attempt PPL elevation/bypass
    3: "Handle_Duplication",    # Steal handle from privileged process
    4: "Minifilter_Unload",     # Unload EDR filesystem minifilter
    5: "Callback_Removal",      # Remove kernel callbacks
    6: "Direct_Syscall",        # NtTerminateProcess via syscall
    7: "Wait_Observe",          # No-op for reconnaissance
}
```

#### **Reward Function (r)**

```python
def calculate_reward(outcome, telemetry_status, stealth_score):
    """
    Reward calculation based on execution outcome
    """
    base_reward = {
        'edr_terminated': 100,           # Full success
        'telemetry_silenced': 75,        # EDR blind but running
        'partial_success': 50,           # Driver loaded/handle obtained
        'no_change': 0,                  # Action had no effect
        'detected_alert': -50,           # EDR generated alert
        'action_blocked': -75,           # EDR actively blocked
        'system_crash': -100,            # BSOD or critical failure
    }[outcome]
    
    # Bonus modifiers
    stealth_bonus = 20 if stealth_score > 0.8 else 0  # No telemetry
    technique_burn = -30 if detected else 0            # Technique now flagged
    
    return base_reward + stealth_bonus + technique_burn
```

### Implementation

**File:** `strategy_selector.py`

```python
import torch
import torch.nn as nn
import numpy as np
from collections import deque
import random

class DQNAgent:
    def __init__(self, state_size=30, action_size=8):
        self.state_size = state_size
        self.action_size = action_size
        self.memory = deque(maxlen=10000)
        self.gamma = 0.95    # Discount factor
        self.epsilon = 1.0   # Exploration rate
        self.epsilon_min = 0.01
        self.epsilon_decay = 0.995
        self.learning_rate = 0.001
        
        self.model = self._build_model()
        self.target_model = self._build_model()
        self.update_target_model()
        
    def _build_model(self):
        """
        Neural network: State -> Q-values for each action
        """
        model = nn.Sequential(
            nn.Linear(self.state_size, 128),
            nn.ReLU(),
            nn.Dropout(0.2),
            nn.Linear(128, 64),
            nn.ReLU(),
            nn.Dropout(0.2),
            nn.Linear(64, self.action_size)
        )
        return model
    
    def select_action(self, state, valid_actions=None):
        """
        Epsilon-greedy action selection
        """
        if np.random.rand() <= self.epsilon:
            # Explore: random action
            return random.choice(valid_actions or range(self.action_size))
        
        # Exploit: best predicted action
        state_tensor = torch.FloatTensor(state).unsqueeze(0)
        with torch.no_grad():
            q_values = self.model(state_tensor)
        
        # Filter invalid actions
        if valid_actions:
            q_values_np = q_values.numpy()[0]
            valid_q = {a: q_values_np[a] for a in valid_actions}
            return max(valid_q, key=valid_q.get)
        
        return torch.argmax(q_values).item()
    
    def remember(self, state, action, reward, next_state, done):
        """Store experience in replay buffer"""
        self.memory.append((state, action, reward, next_state, done))
    
    def replay(self, batch_size=32):
        """
        Train on random batch from memory (Experience Replay)
        """
        if len(self.memory) < batch_size:
            return
        
        minibatch = random.sample(self.memory, batch_size)
        
        states = torch.FloatTensor([e[0] for e in minibatch])
        actions = torch.LongTensor([e[1] for e in minibatch])
        rewards = torch.FloatTensor([e[2] for e in minibatch])
        next_states = torch.FloatTensor([e[3] for e in minibatch])
        dones = torch.FloatTensor([e[4] for e in minibatch])
        
        # Current Q values
        current_q = self.model(states).gather(1, actions.unsqueeze(1))
        
        # Target Q values (from target network)
        with torch.no_grad():
            max_next_q = self.target_model(next_states).max(1)[0]
            target_q = rewards + (1 - dones) * self.gamma * max_next_q
        
        # Loss and backprop
        loss = nn.MSELoss()(current_q.squeeze(), target_q)
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()
        
        # Decay exploration
        if self.epsilon > self.epsilon_min:
            self.epsilon *= self.epsilon_decay
    
    def update_target_model(self):
        """Periodically sync target network"""
        self.target_model.load_state_dict(self.model.state_dict())
    
    def save(self, filepath):
        torch.save({
            'model_state': self.model.state_dict(),
            'epsilon': self.epsilon,
        }, filepath)
    
    def load(self, filepath):
        checkpoint = torch.load(filepath)
        self.model.load_state_dict(checkpoint['model_state'])
        self.epsilon = checkpoint['epsilon']
```

### Usage Example

```python
# Initialize agent
agent = DQNAgent(state_size=30, action_size=8)

# Training loop
for episode in range(1000):
    state = get_system_state()  # From C++ framework
    done = False
    
    while not done:
        # Select action
        action = agent.select_action(state)
        
        # Execute via C++ framework
        outcome = execute_technique(action)
        
        # Calculate reward
        reward = calculate_reward(outcome)
        next_state = get_system_state()
        done = (outcome == 'edr_terminated' or outcome == 'system_crash')
        
        # Store and learn
        agent.remember(state, action, reward, next_state, done)
        agent.replay(batch_size=32)
        
        state = next_state
    
    # Update target network every 10 episodes
    if episode % 10 == 0:
        agent.update_target_model()
        agent.save(f'models/dqn_episode_{episode}.pth')
```

---

## 📦 Component 2: BehaviorAnalyzer

### Purpose
Analyze EDR defensive responses and cluster products by their protection mechanisms.

### Features Extracted

```python
edr_behavior_features = {
    # Detection Timing
    'time_to_detection_ms': float,          # Latency until alert
    'detection_confidence': float,          # Alert severity (0-1)
    
    # Blocking Mechanisms
    'blocking_method': enum[
        'none',                             # No block
        'driver_block',                     # Kernel-mode block
        'process_termination',              # Killed attacking process
        'alert_only',                       # Detected but allowed
    ],
    
    # Defense Layers Active
    'minifilter_present': bool,             # Filesystem filter
    'etw_consumer_active': bool,            # Event Tracing
    'user_mode_hooks': bool,                # IAT/inline hooks
    'kernel_callbacks': list[str],          # Registered callbacks
    
    # Response Actions
    'quarantined_files': list[str],
    'registry_rollback': bool,
    'network_isolation': bool,
    'snapshot_restored': bool,
}
```

### Clustering Algorithm

**File:** `behavior_analyzer.py`

```python
from sklearn.cluster import KMeans
from sklearn.preprocessing import StandardScaler
import pandas as pd

class EDRBehaviorAnalyzer:
    def __init__(self, n_clusters=4):
        self.n_clusters = n_clusters
        self.scaler = StandardScaler()
        self.kmeans = KMeans(n_clusters=n_clusters, random_state=42)
        self.profiles = {}
    
    def analyze_response(self, execution_logs):
        """
        Extract behavioral features from execution logs
        """
        features = {
            'time_to_detection': self._extract_timing(execution_logs),
            'blocking_strength': self._score_blocking(execution_logs),
            'layer_count': self._count_defense_layers(execution_logs),
            'response_severity': self._score_response(execution_logs),
        }
        return features
    
    def cluster_edrs(self, edr_behavior_data):
        """
        Group EDRs by defensive capabilities
        
        Input: DataFrame with columns [edr_name, time_to_detection, 
                                       blocking_strength, layer_count, ...]
        Output: Cluster labels and profiles
        """
        X = edr_behavior_data.drop('edr_name', axis=1)
        X_scaled = self.scaler.fit_transform(X)
        
        labels = self.kmeans.fit_predict(X_scaled)
        edr_behavior_data['cluster'] = labels
        
        # Create cluster profiles
        for cluster_id in range(self.n_clusters):
            cluster_edrs = edr_behavior_data[
                edr_behavior_data['cluster'] == cluster_id
            ]
            
            self.profiles[cluster_id] = {
                'name': self._name_cluster(cluster_edrs),
                'edrs': list(cluster_edrs['edr_name']),
                'avg_detection_time': cluster_edrs['time_to_detection'].mean(),
                'avg_blocking_strength': cluster_edrs['blocking_strength'].mean(),
                'characteristics': self._describe_cluster(cluster_edrs),
            }
        
        return self.profiles
    
    def _name_cluster(self, cluster_data):
        """Generate descriptive name for cluster"""
        avg_strength = cluster_data['blocking_strength'].mean()
        avg_layers = cluster_data['layer_count'].mean()
        
        if avg_strength > 0.8 and avg_layers > 4:
            return "Enterprise Grade (Strong PPL + Multi-Layer)"
        elif avg_strength > 0.6:
            return "Moderate Protection (Driver-Heavy)"
        elif avg_layers < 3:
            return "User-Mode Focused"
        else:
            return "Basic Detection"
    
    def get_vulnerabilities(self, edr_name):
        """
        Identify weak points in specific EDR
        """
        vulnerabilities = []
        
        edr_data = self.get_edr_data(edr_name)
        
        if not edr_data['minifilter_present']:
            vulnerabilities.append("No filesystem monitoring - vulnerable to file drops")
        
        if not edr_data['kernel_callbacks']:
            vulnerabilities.append("Weak process monitoring - BYOVD likely to succeed")
        
        if edr_data['time_to_detection'] > 5000:  # >5 seconds
            vulnerabilities.append("Slow detection - time window for attacks")
        
        return vulnerabilities
```

### Output Example

```python
{
    'cluster_0': {
        'name': 'Enterprise Grade (Strong PPL + Multi-Layer)',
        'edrs': ['CrowdStrike Falcon', 'Microsoft Defender ATP'],
        'avg_detection_time': 342.5,  # ms
        'avg_blocking_strength': 0.92,
        'characteristics': 'Full PPL, minifilter, ETW, kernel callbacks'
    },
    'cluster_1': {
        'name': 'User-Mode Focused',
        'edrs': ['Sophos Home', 'AVG Free'],
        'avg_detection_time': 2890.1,
        'avg_blocking_strength': 0.45,
        'characteristics': 'Primarily user-mode hooks, weak kernel presence'
    }
}
```

---

## 📦 Component 3: ExplainableAI (XAI)

### Purpose
Explain why attacks succeed or fail using interpretable ML techniques (SHAP/LIME).

### Why Explainability Matters

1. **Research Insight**: Understand which EDR features are most protective
2. **Defensive Guidance**: Help EDR vendors improve weak points
3. **Trust**: Researchers can verify ML decisions aren't arbitrary
4. **Debugging**: Identify when model is making bad predictions

### Implementation

**File:** `explainable_ai.py`

```python
import shap
from lime.lime_tabular import LimeTabularExplainer
import matplotlib.pyplot as plt

class EDRExplainer:
    def __init__(self, model, feature_names):
        self.model = model
        self.feature_names = feature_names
        self.shap_explainer = shap.DeepExplainer(model, background_data)
        self.lime_explainer = LimeTabularExplainer(
            training_data,
            feature_names=feature_names,
            class_names=['Fail', 'Success'],
            mode='classification'
        )
    
    def explain_failure(self, technique_id, edr_name, state, outcome):
        """
        Generate comprehensive explanation for why attack failed
        """
        # Get SHAP values
        shap_values = self.shap_explainer.shap_values(state)
        
        # Sort features by importance
        feature_importance = list(zip(
            self.feature_names, 
            np.abs(shap_values[0])
        ))
        feature_importance.sort(key=lambda x: x[1], reverse=True)
        
        # Generate report
        report = {
            'technique': technique_id,
            'edr': edr_name,
            'outcome': outcome,
            'top_blocking_features': [
                {
                    'feature': name,
                    'importance': float(imp),
                    'value': state[self.feature_names.index(name)],
                }
                for name, imp in feature_importance[:5]
            ],
            'explanation': self._generate_narrative(feature_importance, state, outcome),
            'visualization': self._create_force_plot(shap_values, state),
        }
        
        return report
    
    def _generate_narrative(self, feature_importance, state, outcome):
        """
        Human-readable explanation
        """
        top_feature = feature_importance[0][0]
        top_value = state[self.feature_names.index(top_feature)]
        
        narratives = {
            'ppl_protection': {
                True: "EDR process was protected by PPL-Light, preventing handle escalation",
                False: "No PPL protection - attack should have succeeded (investigate failure)"
            },
            'driver_sig_enforcement': {
                True: "Driver signature enforcement blocked unsigned driver load",
                False: "DSE disabled - vulnerable driver load should succeed"
            },
            'minifilter_present': {
                True: "Active filesystem minifilter caught malicious file operations",
                False: "No minifilter - file-based attacks should work"
            }
        }
        
        return narratives.get(top_feature, {}).get(
            top_value, 
            f"Primary factor: {top_feature} = {top_value}"
        )
    
    def feature_importance_global(self, test_data):
        """
        Across all EDRs, rank features by importance
        """
        shap_values = self.shap_explainer.shap_values(test_data)
        
        # Average absolute SHAP value per feature
        mean_abs_shap = np.abs(shap_values).mean(axis=0)
        
        importance_df = pd.DataFrame({
            'feature': self.feature_names,
            'importance': mean_abs_shap
        }).sort_values('importance', ascending=False)
        
        # Visualize
        plt.figure(figsize=(10, 6))
        plt.barh(importance_df['feature'][:10], importance_df['importance'][:10])
        plt.xlabel('Mean |SHAP value| (average impact on model output)')
        plt.title('Top 10 Most Important Features for EDR Blocking')
        plt.tight_layout()
        plt.savefig('feature_importance_global.png')
        
        return importance_df
    
    def compare_techniques(self, technique_a, technique_b, edr_name):
        """
        Explain why technique A worked but technique B failed on same EDR
        """
        state_a = self.get_state_for_technique(technique_a, edr_name)
        state_b = self.get_state_for_technique(technique_b, edr_name)
        
        shap_a = self.shap_explainer.shap_values(state_a)[0]
        shap_b = self.shap_explainer.shap_values(state_b)[0]
        
        diff = shap_a - shap_b
        
        significant_diffs = [
            (self.feature_names[i], diff[i])
            for i in np.argsort(np.abs(diff))[-5:]
        ]
        
        return {
            'comparison': f"{technique_a} vs {technique_b} on {edr_name}",
            'key_differences': significant_diffs,
            'explanation': self._explain_difference(significant_diffs)
        }
```

### Example Output

```json
{
  "technique": "BYOVD_RTCore",
  "edr": "Microsoft Defender",
  "outcome": "action_blocked",
  "top_blocking_features": [
    {
      "feature": "ppl_protection",
      "importance": 0.42,
      "value": true
    },
    {
      "feature": "driver_sig_enforcement",
      "importance": 0.31,
      "value": true
    },
    {
      "feature": "secure_boot",
      "importance": 0.18,
      "value": true
    }
  ],
  "explanation": "EDR process was protected by PPL-Light, preventing handle escalation. Additionally, driver signature enforcement blocked the loading of RTCore64.sys. Secure Boot prevented kernel memory patching. Combined defense-in-depth stopped the attack.",
  "recommendation": "Try PPL bypass technique first, or attempt handle duplication from a privileged service."
}
```

---

## 📦 Component 4: AdaptiveLearner

### Purpose
Continuously improve the model through online learning, transfer learning, and failure analysis.

### Implementation

**File:** `adaptive_learner.py`

```python
import torch
import copy
from datetime import datetime

class AdaptiveLearner:
    def __init__(self, base_agent):
        self.agent = base_agent
        self.failure_patterns = {}
        self.edr_similarity_matrix = {}
        self.learning_history = []
    
    def online_update(self, state, action, reward, next_state, done):
        """
        Real-time model update after each execution
        """
        # Store in replay buffer
        self.agent.remember(state, action, reward, next_state, done)
        
        # Immediate training (vs batched offline training)
        if len(self.agent.memory) >= 4:  # Small batch size for online
            self.agent.replay(batch_size=4)
        
        # Log for analysis
        self.learning_history.append({
            'timestamp': datetime.now(),
            'action': action,
            'reward': reward,
            'epsilon': self.agent.epsilon,
        })
    
    def transfer_learning(self, source_edr, target_edr, similarity_threshold=0.7):
        """
        Transfer knowledge from one EDR to another similar EDR
        
        Example: Use CrowdStrike model as starting point for SentinelOne
        """
        similarity = self.calculate_edr_similarity(source_edr, target_edr)
        
        if similarity < similarity_threshold:
            print(f"EDRs too different (similarity={similarity:.2f}), training from scratch")
            return False
        
        print(f"Transferring knowledge: {source_edr} -> {target_edr} (similarity={similarity:.2f})")
        
        # Load source EDR's trained model
        source_model_path = f"models/{source_edr}_trained.pth"
        target_agent = copy.deepcopy(self.agent)
        target_agent.load(source_model_path)
        
        # Reduce epsilon for fine-tuning (less exploration needed)
        target_agent.epsilon = 0.3
        
        # Fine-tune on target EDR with smaller learning rate
        original_lr = target_agent.learning_rate
        target_agent.learning_rate = original_lr * 0.1
        
        print(f"Fine-tuning on {target_edr} data...")
        # Training happens via normal execution loop
        
        return target_agent
    
    def calculate_edr_similarity(self, edr_a, edr_b):
        """
        Compute similarity score between two EDRs based on:
        - Same vendor family (e.g., both Microsoft)
        - Similar defensive layers
        - Comparable blocking patterns
        """
        features_a = self.get_edr_features(edr_a)
        features_b = self.get_edr_features(edr_b)
        
        # Jaccard similarity on defense layers
        layers_a = set(features_a['defense_layers'])
        layers_b = set(features_b['defense_layers'])
        layer_similarity = len(layers_a & layers_b) / len(layers_a | layers_b)
        
        # Vendor similarity
        vendor_similarity = 1.0 if self.same_vendor(edr_a, edr_b) else 0.0
        
        # Blocking strength similarity
        strength_diff = abs(features_a['blocking_strength'] - features_b['blocking_strength'])
        strength_similarity = 1.0 - strength_diff
        
        # Weighted average
        return (
            0.5 * layer_similarity +
            0.3 * strength_similarity +
            0.2 * vendor_similarity
        )
    
    def analyze_failure_patterns(self, execution_history):
        """
        Identify recurring failure modes to avoid wasted attempts
        """
        failures = [e for e in execution_history if e['reward'] < 0]
        
        # Group failures by (EDR, Technique) pairs
        failure_counts = {}
        for failure in failures:
            key = (failure['edr'], failure['technique'])
            failure_counts[key] = failure_counts.get(key, 0) + 1
        
        # Identify persistent failures (3+ consecutive failures)
        blacklist = []
        for (edr, technique), count in failure_counts.items():
            if count >= 3:
                print(f"⚠️  Blacklisting {technique} for {edr} (failed {count} times)")
                blacklist.append((edr, technique))
                
                # Analyze why it keeps failing
                failure_logs = [
                    f for f in failures 
                    if f['edr'] == edr and f['technique'] == technique
                ]
                common_features = self._find_common_features(failure_logs)
                
                self.failure_patterns[(edr, technique)] = {
                    'failure_count': count,
                    'common_blockers': common_features,
                    'recommended_alternative': self._suggest_alternative(technique, edr)
                }
        
        return blacklist
    
    def _find_common_features(self, failure_logs):
        """
        Find which state features are consistently present in failures
        """
        feature_presence = {}
        for log in failure_logs:
            for feature, value in log['state'].items():
                if value:  # Only track True/present features
                    feature_presence[feature] = feature_presence.get(feature, 0) + 1
        
        # Features present in >80% of failures
        threshold = len(failure_logs) * 0.8
        common = [f for f, count in feature_presence.items() if count >= threshold]
        
        return common
    
    def _suggest_alternative(self, failed_technique, edr):
        """
        Suggest alternative technique based on EDR's weak points
        """
        alternatives = {
            'BYOVD_RTCore': ['PPL_Bypass', 'Handle_Duplication'],
            'PPL_Bypass': ['BYOVD_DBUtil', 'Callback_Removal'],
            'Direct_Syscall': ['Handle_Duplication', 'Minifilter_Unload'],
        }
        
        # Check which alternatives have succeeded before on this EDR
        history = self.get_edr_history(edr)
        successful_on_edr = [
            h['technique'] for h in history if h['reward'] > 50
        ]
        
        # Prioritize alternatives that worked on this EDR
        for alt in alternatives.get(failed_technique, []):
            if alt in successful_on_edr:
                return alt
        
        return alternatives.get(failed_technique, ['Wait_Observe'])[0]
    
    def save_checkpoint(self, episode):
        """
        Save model + learning state
        """
        checkpoint = {
            'episode': episode,
            'model': self.agent.model.state_dict(),
            'optimizer': self.agent.optimizer.state_dict(),
            'epsilon': self.agent.epsilon,
            'failure_patterns': self.failure_patterns,
            'learning_history': self.learning_history,
        }
        torch.save(checkpoint, f'checkpoints/adaptive_ep{episode}.pth')
    
    def load_checkpoint(self, filepath):
        """Load saved state"""
        checkpoint = torch.load(filepath)
        self.agent.model.load_state_dict(checkpoint['model'])
        self.agent.epsilon = checkpoint['epsilon']
        self.failure_patterns = checkpoint['failure_patterns']
        self.learning_history = checkpoint['learning_history']
```

---

## 🔗 Integration with C++ Framework

### Python-C++ Bridge (pybind11)

**File:** `ml_bridge.cpp`

```cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

class MLBridge {
public:
    MLBridge() {
        // Initialize Python interpreter
        py::initialize_interpreter();
        
        // Import ML modules
        py::module_ sys = py::module_::import("sys");
        sys.attr("path").attr("append")("./ml_framework");
        
        strategy_selector_ = py::module_::import("strategy_selector");
        behavior_analyzer_ = py::module_::import("behavior_analyzer");
        explainer_ = py::module_::import("explainable_ai");
        
        // Create agent instance
        auto agent_class = strategy_selector_.attr("DQNAgent");
        agent_ = agent_class(30, 8);  // state_size=30, action_size=8
    }
    
    int selectBestTechnique(const std::map<std::string, double>& state) {
        // Convert C++ state to Python
        py::dict py_state;
        for (const auto& [key, value] : state) {
            py_state[py::cast(key)] = py::cast(value);
        }
        
        // Call RL agent
        py::object action = agent_.attr("select_action")(py_state);
        return action.cast<int>();
    }
    
    void updateModel(const std::map<std::string, double>& state,
                     int action,
                     double reward,
                     const std::map<std::string, double>& next_state,
                     bool done) {
        // Convert to Python and update
        agent_.attr("remember")(
            convertState(state),
            action,
            reward,
            convertState(next_state),
            done
        );
        
        agent_.attr("replay")(32);  // Batch size
    }
    
    std::string explainOutcome(const ExecutionResult& result) {
        auto explainer = explainer_.attr("EDRExplainer")(agent_);
        py::dict explanation = explainer.attr("explain_failure")(
            result.technique_id,
            result.edr_name,
            convertState(result.state),
            result.outcome
        );
        
        return py::str(explanation).cast<std::string>();
    }

private:
    py::module_ strategy_selector_;
    py::module_ behavior_analyzer_;
    py::module_ explainer_;
    py::object agent_;
    
    py::dict convertState(const std::map<std::string, double>& state) {
        py::dict py_state;
        for (const auto& [key, value] : state) {
            py_state[py::cast(key)] = py::cast(value);
        }
        return py_state;
    }
};
```

### Usage in ml_engine.cpp

```cpp
// In ml_engine.cpp - add to existing file

#include "ml_bridge.cpp"

class MLEngine {
private:
    std::unique_ptr<MLBridge> ml_bridge_;

public:
    bool initialize() {
        ml_bridge_ = std::make_unique<MLBridge>();
        std::cout << "[*] ML Bridge initialized" << std::endl;
        return true;
    }
    
    std::string selectTechnique(const SystemState& system_state, 
                               const EDRInfo& edr_info) {
        // Build state map
        std::map<std::string, double> state;
        state["edr_process_running"] = edr_info.processRunning ? 1.0 : 0.0;
        state["edr_driver_loaded"] = edr_info.driverLoaded ? 1.0 : 0.0;
        state["ppl_protection"] = system_state.pplEnabled ? 1.0 : 0.0;
        // ... add all 30 state features
        
        // Get action from RL agent
        int action_id = ml_bridge_->selectBestTechnique(state);
        
        // Map to technique name
        const std::vector<std::string> techniques = {
            "BYOVD_RTCore", "BYOVD_DBUtil", "PPL_Bypass",
            "Handle_Duplication", "Minifilter_Unload",
            "Callback_Removal", "Direct_Syscall", "Wait_Observe"
        };
        
        return techniques[action_id];
    }
    
    void processFeedback(const ExecutionResult& result) {
        // Calculate reward
        double reward = calculateReward(result);
        
        // Update ML model
        ml_bridge_->updateModel(
            result.initial_state,
            result.action_id,
            reward,
            result.final_state,
            result.done
        );
        
        // Generate XAI explanation if failed
        if (reward < 0) {
            std::string explanation = ml_bridge_->explainOutcome(result);
            std::cout << "[XAI] " << explanation << std::endl;
        }
    }
};
```

---

## 📊 Evaluation Metrics

### Model Performance

| Metric | Description | Target | Measurement |
|--------|-------------|--------|-------------|
| **Success Rate** | % of EDR terminations successful | >60% | Episodes ending with reward +100 |
| **Sample Efficiency** | Avg techniques tried before success | <4 | Actions per episode |
| **Prediction Accuracy** | Model predicts best technique | >70% | Top-3 actions contain success |
| **Learning Speed** | Episodes to convergence | <200 | When success rate plateaus |
| **Transfer Efficiency** | Speedup with transfer learning | >50% | Fewer trials on similar EDR |

### XAI Quality

| Metric | Evaluation Method |
|--------|------------------|
| **Explanation Fidelity** | SHAP values match ground truth (manual verification) |
| **Human Comprehension** | User study: researchers rate clarity 1-5 |
| **Feature Stability** | Top features consistent across similar states |

---

## 🗂️ File Structure

```
src/ml_framework/
├── DESIGN.md                    # This file
├── README.md                    # Quick start guide
├── requirements.txt             # Python dependencies
│
├── models/                      # Saved RL models
│   ├── dqn_defender.pth
│   ├── dqn_crowdstrike.pth
│   └── transfer_base.pth
│
├── data/                        # Training data
│   ├── execution_logs.csv
│   ├── edr_profiles.json
│   └── failure_patterns.json
│
├── python/                      # Python ML code
│   ├── strategy_selector.py    # DQN/PPO agent
│   ├── behavior_analyzer.py    # EDR clustering
│   ├── explainable_ai.py       # SHAP/LIME
│   ├── adaptive_learner.py     # Online learning
│   └── utils.py                # Helper functions
│
├── ml_bridge.cpp               # Python-C++ interface
├── ml_engine.cpp               # Existing C++ code
└── ml_engine.hpp               # Existing header
```

---

## 🚀 Quick Start

### 1. Install Dependencies

```bash
pip install -r src/ml_framework/requirements.txt
```

**requirements.txt:**
```
torch>=2.0.0
numpy>=1.24.0
pandas>=2.0.0
scikit-learn>=1.3.0
shap>=0.42.0
lime>=0.2.0.1
matplotlib>=3.7.0
pybind11>=2.11.0
```

### 2. Train Initial Model

```bash
cd src/ml_framework/python
python train_agent.py --edr defender --episodes 500
```

### 3. Test Model

```bash
python test_agent.py --edr defender --model models/dqn_defender.pth
```

### 4. Generate XAI Report

```bash
python explain_failures.py --logs data/execution_logs.csv --output reports/
```

---

## 📚 Research Papers Reference

1. **Reinforcement Learning for Security**: Behzadan & Munir (2018) - "Adversarial RL in Cybersecurity"
2. **Explainable AI**: Lundberg & Lee (2017) - "A Unified Approach to Interpreting Model Predictions" (SHAP)
3. **Transfer Learning**: Pan & Yang (2010) - "A Survey on Transfer Learning"
4. **EDR Evasion Techniques**: Botacin et al. (2021) - "Windows EDR Internals"

---

## ⚠️ Ethical Considerations

This framework is for **defensive research only**:

- ✅ Test your organization's EDR products
- ✅ Academic study in isolated labs
- ✅ Improve EDR defensive capabilities
- ❌ Unauthorized testing on third-party systems
- ❌ Malicious use against production EDRs

**Always obtain proper authorization before testing.**

---

## 📧 Questions?

Contact: Karthik (ML Framework Lead)

Happy Learning! 🧠🛡️
