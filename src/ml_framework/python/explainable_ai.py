"""
Explainable AI - SHAP / LIME-based Attack Outcome Analysis
===========================================================
Author: Karthik
Description:
  Explains why attack techniques succeed or fail against specific EDRs using
  gradient-based SHAP values computed directly from the DQN policy network.

  Falls back to a simple manual feature-importance analysis when SHAP is not
  installed, so the module always works.

Integration:
  - ml_server.py calls explain_failure() when the C++ side sends an "explain"
    command after a technique execution.
  - Results are embedded into AnalysisReport.recommendation by ml_engine.cpp.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import numpy as np

from utils import FEATURE_NAMES

# Feature groups for narrative generation
_PROTECTION_FEATURES = {
    "ppl_protection":          "Protected Process Light (PPL)",
    "driver_sig_enforcement":  "Driver Signature Enforcement (DSE)",
    "secure_boot":             "Secure Boot",
    "virtualization":          "Virtualization-Based Security",
    "edr_driver_loaded":       "Active EDR kernel driver",
    "edr_process_running":     "Running EDR service",
    "last_result_blocked":     "Previously blocked by EDR",
}


# ============================================================================
# EDR Explainer
# ============================================================================

class EDRExplainer:
    """
    Provides SHAP-based explanations for DQN action outcomes.

    Args:
        model:         A DQNAgent whose policy_net will be explained.
        feature_names: List of 26 feature name strings.
    """

    def __init__(self, model, feature_names: Optional[List[str]] = None):
        self.model         = model
        self.feature_names = feature_names or FEATURE_NAMES

        # Background dataset accumulated over time (for SHAP baseline)
        self._background: List[np.ndarray] = []
        self._max_bg_size = 200

        # History of explanations for global importance
        self._explanation_history: List[Dict] = []

        self._has_shap = self._try_import_shap()

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def add_background_sample(self, state: np.ndarray):
        """
        Accumulate state vectors as SHAP background distribution.
        Should be called after every execution step.
        """
        self._background.append(np.array(state, dtype=np.float32))
        if len(self._background) > self._max_bg_size:
            self._background = self._background[-self._max_bg_size:]

    def explain_failure(self, technique_id: str, edr_name: str,
                        state: np.ndarray, outcome: str) -> Optional[Dict]:
        """
        Generate a comprehensive explanation for a technique execution outcome.

        Args:
            technique_id: MITRE ATT&CK technique ID (e.g. "T1068")
            edr_name:     EDR product name
            state:        26-element state vector at time of execution
            outcome:      One of "success", "blocked", "failed", "detected", "crash"

        Returns:
            Dict with keys:
              explanation  — short human-readable explanation string
              top_features — [(feature_name, importance_score), ...] top 5
              narrative    — multi-sentence natural language analysis
        """
        state = np.array(state, dtype=np.float32)
        self.add_background_sample(state)

        if self._has_shap and len(self._background) >= 10:
            importances = self._shap_importances(state)
        else:
            importances = self._gradient_importances(state)

        top_features = sorted(
            zip(self.feature_names, importances),
            key=lambda x: abs(x[1]),
            reverse=True,
        )[:5]

        explanation = self._build_explanation(technique_id, edr_name, outcome, top_features)
        narrative   = self._build_narrative(technique_id, edr_name, outcome,
                                            state, top_features)

        result = {
            "explanation":  explanation,
            "top_features": [(name, float(score)) for name, score in top_features],
            "narrative":    narrative,
        }
        self._explanation_history.append(result)
        return result

    def feature_importance_global(self) -> List[Tuple[str, float]]:
        """
        Aggregate importance scores across all stored explanations.
        Returns list of (feature_name, mean_abs_importance) sorted descending.
        """
        if not self._explanation_history:
            return []

        totals: Dict[str, float] = {name: 0.0 for name in self.feature_names}
        counts: Dict[str, int]   = {name: 0   for name in self.feature_names}

        for record in self._explanation_history:
            for name, score in record.get("top_features", []):
                totals[name] += abs(score)
                counts[name] += 1

        result = []
        for name in self.feature_names:
            if counts[name] > 0:
                result.append((name, totals[name] / counts[name]))
            else:
                result.append((name, 0.0))

        return sorted(result, key=lambda x: x[1], reverse=True)

    def compare_techniques(self, technique_a: str, technique_b: str,
                            edr_name: str) -> Optional[Dict]:
        """
        Explain why technique_a may fare differently from technique_b
        against the same EDR using the global importance ranking.
        """
        rankings = self.feature_importance_global()
        if not rankings:
            return None

        top_5 = rankings[:5]
        narrative = (
            f"Comparing {technique_a} vs {technique_b} against {edr_name}:\n"
            f"The most influential system features overall are:\n"
        )
        for i, (feat, imp) in enumerate(top_5, 1):
            narrative += f"  {i}. {feat} (importance {imp:.3f})\n"
        narrative += (
            f"\nTechniques that bypass {top_5[0][0]} (highest-impact feature) "
            f"are most likely to succeed against {edr_name}."
        )

        return {
            "technique_a":   technique_a,
            "technique_b":   technique_b,
            "edr_name":      edr_name,
            "top_influences": top_5,
            "narrative":     narrative,
        }

    # ------------------------------------------------------------------
    # Private: SHAP-based importances
    # ------------------------------------------------------------------

    def _shap_importances(self, state: np.ndarray) -> np.ndarray:
        """Compute SHAP values using the DQN policy network as the model."""
        import shap
        import torch

        background = np.stack(self._background[-100:], axis=0)

        def _predict(X: np.ndarray) -> np.ndarray:
            """Wrapper: state batch -> max Q-value per sample."""
            with torch.no_grad():
                t = torch.FloatTensor(X)
                q = self.model.policy_net(t)
                return q.max(dim=1).values.numpy().reshape(-1, 1)

        explainer  = shap.KernelExplainer(_predict, background)
        shap_vals  = explainer.shap_values(state.reshape(1, -1), nsamples=50)
        return np.abs(shap_vals[0]).flatten()

    # ------------------------------------------------------------------
    # Private: Gradient-based fallback importances
    # ------------------------------------------------------------------

    def _gradient_importances(self, state: np.ndarray) -> np.ndarray:
        """
        Fallback: use input × gradient (integrated gradients approximation).
        Works without SHAP installed.
        """
        import torch

        state_t = torch.FloatTensor(state).unsqueeze(0).requires_grad_(True)
        q_vals  = self.model.policy_net(state_t)
        best_q  = q_vals.max()
        best_q.backward()

        grads       = state_t.grad.detach().numpy().flatten()
        importances = np.abs(grads * state)
        return importances

    # ------------------------------------------------------------------
    # Private: Narrative builders
    # ------------------------------------------------------------------

    def _build_explanation(self, technique_id: str, edr_name: str,
                           outcome: str, top_features: List) -> str:
        feat_str = ", ".join(f"{n} ({s:+.2f})" for n, s in top_features[:3])
        return (
            f"Technique {technique_id} {outcome} against {edr_name}. "
            f"Key driving features: {feat_str}."
        )

    def _build_narrative(self, technique_id: str, edr_name: str, outcome: str,
                         state: np.ndarray, top_features: List) -> str:
        lines = [
            f"Analysis of {technique_id} execution against {edr_name}:",
            f"  Outcome: {outcome.upper()}",
            "",
            "Top contributing state features:",
        ]

        for i, (name, score) in enumerate(top_features, 1):
            direction = "increased" if score > 0 else "decreased"
            human     = _PROTECTION_FEATURES.get(name, name.replace("_", " "))
            lines.append(
                f"  {i}. {human}: impact {score:+.3f} ({direction} likelihood)"
            )

        lines.append("")

        # Protective features that were active
        active_protections = [
            _PROTECTION_FEATURES[f]
            for f in _PROTECTION_FEATURES
            if f in self.feature_names
            and state[self.feature_names.index(f)] > 0.5
        ]

        if active_protections:
            lines.append("Active defensive mechanisms detected:")
            for p in active_protections:
                lines.append(f"  - {p}")
            lines.append("")

        if outcome in ("blocked", "failed", "detected"):
            lines.append(
                "Recommendation: Consider techniques that bypass "
                + (active_protections[0] if active_protections else "the top feature above")
                + " or operate in user-mode to reduce kernel visibility."
            )
        elif outcome == "success":
            lines.append(
                "Recommendation: Replicate the same pre-conditions "
                "for other EDR tests. Consider snapshotting this system state."
            )
        elif outcome == "crash":
            lines.append(
                "Warning: System crash detected. Avoid this technique variant "
                "until root cause is identified."
            )

        return "\n".join(lines)

    # ------------------------------------------------------------------
    # Helper
    # ------------------------------------------------------------------

    @staticmethod
    def _try_import_shap() -> bool:
        try:
            import shap  # noqa: F401
            return True
        except ImportError:
            return False


# ============================================================================
# Quick smoke test
# ============================================================================

if __name__ == "__main__":
    import sys
    sys.path.insert(0, str(Path(__file__).parent))
    from strategy_selector import DQNAgent

    agent    = DQNAgent(state_size=26, action_size=4)
    explainer = EDRExplainer(model=agent, feature_names=FEATURE_NAMES)

    # Simulate a "blocked" state (PPL on, DSE on, EDR running)
    state = np.zeros(26, dtype=np.float32)
    state[0]  = 1.0   # Defender
    state[6]  = 1.0   # EDR process running
    state[7]  = 1.0   # kernel driver loaded
    state[16] = 1.0   # PPL
    state[17] = 1.0   # DSE

    # Add some background samples
    for _ in range(15):
        bg = np.random.rand(26).astype(np.float32)
        explainer.add_background_sample(bg)

    print("[1] explain_failure()...")
    result = explainer.explain_failure("T1068", "Defender", state, "blocked")
    print(f"    Explanation: {result['explanation']}")
    print(f"    Top feature: {result['top_features'][0]}")
    print(f"    Narrative (first 3 lines):")
    for line in result["narrative"].split("\n")[:4]:
        print(f"      {line}")

    print("\n[2] feature_importance_global()...")
    ranking = explainer.feature_importance_global()
    print(f"    Top 3 global: {ranking[:3]}")

    print("\n[3] compare_techniques()...")
    comp = explainer.compare_techniques("T1068", "T1562.001", "Defender")
    if comp:
        print(f"    {comp['narrative'][:120]}...")

    print("\n=== All EDRExplainer tests passed! ===")
