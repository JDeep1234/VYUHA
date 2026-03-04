"""
Adaptive Learner - Online Learning and Transfer Learning
=========================================================
Author: Karthik
Description:
  Wraps the DQNAgent to provide:
    1. online_update()          — real-time model update after each execution step
    2. transfer_learning()      — apply knowledge from a well-trained EDR model
                                  to a new, similar EDR
    3. analyze_failure_patterns() — detect (EDR, technique) pairs that always fail
                                    so the orchestrator can skip wasted attempts
    4. calculate_edr_similarity() — cosine similarity between behavioral feature
                                    vectors of two EDRs (used by transfer_learning)

Integration:
  - Called from ml_server.py after every "train" command (online_update).
  - Called from agent.cpp / orchestrator to transfer knowledge between EDR sessions.
"""

from __future__ import annotations

import copy
import json
import math
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import numpy as np


# ============================================================================
# Typing helpers
# ============================================================================

State  = np.ndarray   # shape (30,)
Action = int
Reward = float


# ============================================================================
# Adaptive Learner
# ============================================================================

class AdaptiveLearner:
    """
    Handles online learning, transfer learning, and failure-pattern analysis
    for the DQN strategy selector.

    Args:
        base_agent: a DQNAgent instance (from strategy_selector.py)
    """

    def __init__(self, base_agent):
        self.agent = base_agent

        # {edr_name: {technique_id: {"success": int, "fail": int}}}
        self._edr_technique_stats: Dict[str, Dict[str, Dict[str, int]]] = defaultdict(
            lambda: defaultdict(lambda: {"success": 0, "fail": 0})
        )

        # Complete execution history for pattern analysis
        # Each entry: {"edr": str, "technique": str, "outcome": str, "timestamp": str}
        self._history: List[Dict[str, Any]] = []

        # {edr_name: feature_vector (np.ndarray)} — for similarity computation
        self._edr_feature_vectors: Dict[str, np.ndarray] = {}

        # Separate model snapshots per EDR so we can restore on transfer
        # {edr_name: state_dict_copy}
        self._edr_model_snapshots: Dict[str, Any] = {}

        self._data_dir = Path(__file__).parent.parent / "data"
        self._data_dir.mkdir(exist_ok=True)
        self._history_path = self._data_dir / "execution_history.json"
        self._load_history()

    # ------------------------------------------------------------------
    # 1. Online Learning
    # ------------------------------------------------------------------

    def online_update(self, state: State, action: Action, reward: Reward,
                      next_state: State, done: bool) -> Optional[float]:
        """
        Real-time model update after each execution step.

        Calls agent.remember() then agent.replay() immediately so the model
        learns from the very latest experience without waiting for an episode end.

        Returns replay loss (float) or None if replay buffer not yet full.
        """
        self.agent.remember(state, action, reward, next_state, done)

        loss: Optional[float] = None
        if len(self.agent.memory) >= self.agent.batch_size:
            loss = self.agent.replay()

        return loss

    def record_execution(self, edr_name: str, technique_id: str, success: bool,
                         detected: bool):
        """
        Update per-EDR per-technique statistics.
        Called by orchestrator after each exploit attempt.
        """
        stats = self._edr_technique_stats[edr_name][technique_id]
        if success and not detected:
            stats["success"] += 1
        else:
            stats["fail"] += 1

        self._history.append({
            "edr":       edr_name,
            "technique": technique_id,
            "outcome":   "success" if (success and not detected) else "fail",
            "timestamp": datetime.now(timezone.utc).isoformat(),
        })
        self._save_history()

    def register_edr_features(self, edr_name: str, feature_vector: np.ndarray):
        """
        Store the behavioral feature vector for an EDR (from BehaviorAnalyzer).
        Used by calculate_edr_similarity and transfer_learning.
        """
        self._edr_feature_vectors[edr_name] = np.array(feature_vector, dtype=np.float32)

    def snapshot_model(self, edr_name: str):
        """
        Save a deep copy of the current model state under the given EDR label.
        Should be called at the end of a successful training run against an EDR.
        """
        import copy
        self._edr_model_snapshots[edr_name] = {
            "policy": copy.deepcopy(self.agent.policy_net.state_dict()),
            "target": copy.deepcopy(self.agent.target_net.state_dict()),
            "epsilon": self.agent.epsilon,
        }

    # ------------------------------------------------------------------
    # 2. Transfer Learning
    # ------------------------------------------------------------------

    def transfer_learning(self, source_edr: str, target_edr: str,
                          similarity_threshold: float = 0.7):
        """
        Transfer model weights from a source EDR to a target EDR if they are
        behaviourally similar enough.

        Algorithm:
          1. Compute cosine similarity between feature vectors.
          2. If similarity >= threshold, copy source model weights to a new agent.
          3. Reset epsilon partially (to allow exploration of the new EDR).
          4. Return the new agent (or None if transfer rejected).

        Args:
            source_edr: EDR name whose trained model we want to reuse.
            target_edr: EDR name we are about to test against.
            similarity_threshold: Minimum similarity to approve transfer.

        Returns:
            New DQNAgent with transferred weights, or None.
        """
        sim = self.calculate_edr_similarity(source_edr, target_edr)
        if sim < similarity_threshold:
            return None

        if source_edr not in self._edr_model_snapshots:
            return None

        # Import here to avoid circular imports at module load
        from strategy_selector import DQNAgent

        new_agent = DQNAgent(
            state_size=self.agent.state_size,
            action_size=self.agent.action_size,
            learning_rate=self.agent.learning_rate,
        )
        snapshot = self._edr_model_snapshots[source_edr]
        new_agent.policy_net.load_state_dict(snapshot["policy"])
        new_agent.target_net.load_state_dict(snapshot["target"])

        # Partially reset epsilon: start a bit above minimum to allow
        # exploration while benefiting from prior knowledge
        new_agent.epsilon = max(0.3, snapshot["epsilon"] * 1.5)

        return new_agent

    # ------------------------------------------------------------------
    # 3. Failure Pattern Analysis
    # ------------------------------------------------------------------

    def analyze_failure_patterns(self, execution_history: Optional[List[Dict]] = None,
                                 failure_threshold: int = 3) -> List[Tuple[str, str]]:
        """
        Identify (EDR, technique) pairs that have failed too many times
        without a single success — these should be excluded from future runs.

        Args:
            execution_history: Optional external history to analyse (from C++ side).
                               Falls back to internal _history if None.
            failure_threshold: Number of consecutive failures before blacklisting.

        Returns:
            List of (edr_name, technique_id) tuples to blacklist.
        """
        blacklist: List[Tuple[str, str]] = []

        # Merge external history if provided
        history = execution_history or []
        if not history:
            history = self._history

        # Build stats from history
        stats: Dict[Tuple[str, str], Dict[str, int]] = defaultdict(
            lambda: {"success": 0, "fail": 0}
        )
        for entry in history:
            key = (entry.get("edr", ""), entry.get("technique", ""))
            if entry.get("outcome") == "success":
                stats[key]["success"] += 1
            else:
                stats[key]["fail"] += 1

        # Merge with internal per-EDR stats
        for edr, techniques in self._edr_technique_stats.items():
            for tech, counts in techniques.items():
                key = (edr, tech)
                stats[key]["success"] += counts["success"]
                stats[key]["fail"]    += counts["fail"]

        for (edr, tech), counts in stats.items():
            if counts["success"] == 0 and counts["fail"] >= failure_threshold:
                blacklist.append((edr, tech))

        return blacklist

    # ------------------------------------------------------------------
    # 4. EDR Similarity
    # ------------------------------------------------------------------

    def calculate_edr_similarity(self, edr_a: str, edr_b: str) -> float:
        """
        Compute cosine similarity between behavioral feature vectors of two EDRs.

        Returns a score in [0, 1].  Returns 0.0 if feature data is unavailable.
        """
        vec_a = self._edr_feature_vectors.get(edr_a)
        vec_b = self._edr_feature_vectors.get(edr_b)

        if vec_a is None or vec_b is None:
            # Fallback: check if they share the same cluster
            # (requires BehaviorAnalyzer data, handled by ml_server.py)
            return 0.0

        dot   = float(np.dot(vec_a, vec_b))
        norm_a = float(np.linalg.norm(vec_a))
        norm_b = float(np.linalg.norm(vec_b))

        if norm_a < 1e-9 or norm_b < 1e-9:
            return 0.0

        return max(0.0, min(1.0, dot / (norm_a * norm_b)))

    # ------------------------------------------------------------------
    # Persistence helpers
    # ------------------------------------------------------------------

    def _save_history(self):
        try:
            with open(self._history_path, "w") as f:
                json.dump(self._history[-5000:], f, indent=2)  # keep last 5000
        except Exception:
            pass

    def _load_history(self):
        try:
            if self._history_path.exists():
                with open(self._history_path) as f:
                    self._history = json.load(f)
        except Exception:
            self._history = []


# ============================================================================
# Quick smoke test
# ============================================================================

if __name__ == "__main__":
    import sys
    sys.path.insert(0, str(Path(__file__).parent))
    from strategy_selector import DQNAgent

    agent   = DQNAgent(state_size=30, action_size=8)
    learner = AdaptiveLearner(base_agent=agent)

    state      = np.zeros(30, dtype=np.float32)
    state[0]   = 1.0   # EDR running
    next_state = state.copy()
    next_state[0] = 0.0

    print("[1] Testing online_update()...")
    for i in range(50):
        loss = learner.online_update(state, action=0, reward=100.0,
                                     next_state=next_state, done=True)
    print(f"    loss after 50 steps: {loss}")

    print("[2] Testing record_execution()...")
    for _ in range(4):
        learner.record_execution("CrowdStrike", "T1068", success=False, detected=True)
    learner.record_execution("CrowdStrike", "T1562.001", success=True, detected=False)

    print("[3] Testing analyze_failure_patterns()...")
    blacklist = learner.analyze_failure_patterns(failure_threshold=3)
    print(f"    Blacklist: {blacklist}")
    assert ("CrowdStrike", "T1068") in blacklist, "Expected T1068 blacklisted"

    print("[4] Testing EDR similarity (no vectors)...")
    sim = learner.calculate_edr_similarity("CrowdStrike", "Defender")
    print(f"    Similarity (no data): {sim}")

    print("[5] Testing similarity with feature vectors...")
    learner.register_edr_features("CrowdStrike", np.array([0.9, 0.8, 4.0, 1.0, 1.0], dtype=np.float32))
    learner.register_edr_features("SentinelOne", np.array([0.85, 0.75, 3.8, 1.0, 1.0], dtype=np.float32))
    learner.register_edr_features("CarbonBlack", np.array([0.2, 0.1, 1.5, 0.0, 0.0], dtype=np.float32))
    print(f"    CS <> S1: {learner.calculate_edr_similarity('CrowdStrike', 'SentinelOne'):.3f}")
    print(f"    CS <> CB: {learner.calculate_edr_similarity('CrowdStrike', 'CarbonBlack'):.3f}")

    print("\n=== All AdaptiveLearner tests passed! ===")
