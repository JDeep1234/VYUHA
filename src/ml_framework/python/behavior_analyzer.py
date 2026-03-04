"""
Behavior Analyzer - EDR Response Pattern Recognition
=====================================================
Author: Karthik
Description: Analyse and cluster EDRs by their defensive capability profile
             using K-means on extracted behavioral features.

Integration:
  - Receives execution_logs dicts from ml_server.py (populated by Jdeep's
    orchestrator after each ExploitResult comes back from Bipin's module).
  - Returns cluster profiles and vulnerability lists that feed into
    MLEngine::generateReport() and the ExplainableAI narrative.
"""

from __future__ import annotations

import json
import math
from pathlib import Path
from typing import Any, Dict, List, Optional

import numpy as np
import pandas as pd
from sklearn.cluster import KMeans
from sklearn.preprocessing import StandardScaler


# ============================================================================
# Feature constants
# ============================================================================

BLOCKING_METHODS = ["none", "alert_only", "driver_block", "process_termination"]

_NUMERIC_FEATURES = [
    "time_to_detection_ms",
    "detection_confidence",
    "blocking_strength",       # 0=none, 1=alert, 2=driver, 3=termination (normalized)
    "layer_count",             # number of defensive layers observed
    "response_severity",       # 0-1 composite score
    "minifilter_present",      # bool as float
    "etw_consumer_active",
    "user_mode_hooks",
    "kernel_callbacks_count",
    "network_isolation",
    "registry_rollback",
]


def _safe_float(v: Any, default: float = 0.0) -> float:
    try:
        return float(v)
    except (TypeError, ValueError):
        return default


# ============================================================================
# EDR Behavior Analyzer
# ============================================================================

class EDRBehaviorAnalyzer:
    """
    Clusters EDR products by their observed defense behaviors.

    Lifecycle:
      1. Call analyze_response(execution_logs) after each test run.
      2. Feed result to accumulate_observation(edr_name, row).
      3. Once >= n_clusters observations are accumulated, clustering
         happens automatically on the next accumulate call.
      4. Use get_cluster_id() / get_vulnerabilities() for downstream use.
    """

    def __init__(self, n_clusters: int = 4):
        self.n_clusters = n_clusters
        self.scaler  = StandardScaler()
        self.kmeans  = KMeans(n_clusters=n_clusters, random_state=42, n_init=10)

        # Accumulated observations: list of feature dicts
        self._observations: List[Dict[str, Any]] = []
        self._df: Optional[pd.DataFrame] = None

        # Cluster profiles {cluster_id: {name, edrs, ...}}
        self._profiles: Dict[int, Dict] = {}

        # Per-EDR latest assigned cluster
        self._edr_clusters: Dict[str, int] = {}

        # Paths for persistence
        self._data_dir = Path(__file__).parent.parent / "data"
        self._data_dir.mkdir(exist_ok=True)
        self._obs_path = self._data_dir / "edr_observations.json"

        self._load_observations()

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def analyze_response(self, execution_logs: Dict[str, Any]) -> Dict[str, Any]:
        """
        Extract numerical behavioral features from raw execution_logs.

        execution_logs keys (all optional, defaults applied):
          time_to_detection_ms: float
          detection_confidence: float  (0-1)
          blocking_method: str  ("none"|"alert_only"|"driver_block"|"process_termination")
          minifilter_present: bool
          etw_consumer_active: bool
          user_mode_hooks: bool
          kernel_callbacks: list[str]
          network_isolation: bool
          registry_rollback: bool
          edr_alerts: list[str]
        """
        ttd = _safe_float(execution_logs.get("time_to_detection_ms"), 99999.0)
        confidence = _safe_float(execution_logs.get("detection_confidence"), 0.0)

        bm = execution_logs.get("blocking_method", "none")
        if bm not in BLOCKING_METHODS:
            bm = "none"
        blocking_strength = float(BLOCKING_METHODS.index(bm)) / (len(BLOCKING_METHODS) - 1)

        minifilter  = float(bool(execution_logs.get("minifilter_present", False)))
        etw         = float(bool(execution_logs.get("etw_consumer_active", False)))
        um_hooks    = float(bool(execution_logs.get("user_mode_hooks", False)))
        callbacks   = len(execution_logs.get("kernel_callbacks", []))
        net_iso     = float(bool(execution_logs.get("network_isolation", False)))
        reg_roll    = float(bool(execution_logs.get("registry_rollback", False)))

        layer_count = minifilter + etw + um_hooks + float(callbacks > 0) + net_iso

        # Composite response severity
        response_severity = (confidence + blocking_strength + net_iso + reg_roll) / 4.0

        return {
            "time_to_detection_ms":  ttd,
            "detection_confidence":  confidence,
            "blocking_strength":     blocking_strength,
            "layer_count":           layer_count,
            "response_severity":     response_severity,
            "minifilter_present":    minifilter,
            "etw_consumer_active":   etw,
            "user_mode_hooks":       um_hooks,
            "kernel_callbacks_count": float(callbacks),
            "network_isolation":     net_iso,
            "registry_rollback":     reg_roll,
        }

    def accumulate_observation(self, edr_name: str, feature_row: Dict[str, Any]):
        """
        Add one behavioral observation to the dataset and re-cluster if possible.
        """
        row = dict(feature_row)
        row["edr_name"] = edr_name
        self._observations.append(row)
        self._save_observations()

        # Attempt clustering when we have enough data
        if len(self._observations) >= self.n_clusters:
            self._refit()

    def get_profiles(self) -> Dict[int, Dict]:
        return self._profiles

    def get_cluster_id(self, edr_name: str) -> int:
        return self._edr_clusters.get(edr_name, -1)

    def get_vulnerabilities(self, edr_name: str) -> List[str]:
        """
        Return a list of human-readable weakness strings for a specific EDR.
        """
        vulnerabilities: List[str] = []

        # Find the most recent observation for this EDR
        matches = [r for r in self._observations if r.get("edr_name") == edr_name]
        if not matches:
            return ["No behavioral data collected for this EDR yet."]

        data = matches[-1]  # most recent

        if _safe_float(data.get("time_to_detection_ms"), 0) > 5000:
            vulnerabilities.append(
                f"Slow detection ({data.get('time_to_detection_ms'):.0f} ms) — "
                "meaningful execution window before alert"
            )

        if not _safe_float(data.get("minifilter_present")):
            vulnerabilities.append(
                "No filesystem minifilter detected — file-based drops likely unmonitored"
            )

        if not _safe_float(data.get("kernel_callbacks_count")):
            vulnerabilities.append(
                "No kernel callbacks registered — BYOVD callback removal may go undetected"
            )

        if _safe_float(data.get("blocking_strength"), 0) < 0.4:
            vulnerabilities.append(
                "Weak blocking capability (alert-only mode) — "
                "techniques may execute fully before intervention"
            )

        if not _safe_float(data.get("etw_consumer_active")):
            vulnerabilities.append(
                "No ETW consumer observed — ETW patching attacks likely effective"
            )

        if not vulnerabilities:
            vulnerabilities.append("No significant weaknesses identified based on current data")

        return vulnerabilities

    def cluster_edrs(self, edr_behavior_data: pd.DataFrame) -> Dict[int, Dict]:
        """
        Cluster a provided DataFrame of EDR behavior profiles.
        Columns: edr_name + numeric feature columns.
        """
        feature_cols = [c for c in _NUMERIC_FEATURES if c in edr_behavior_data.columns]
        X = edr_behavior_data[feature_cols].fillna(0).values

        if len(X) < self.n_clusters:
            raise ValueError(f"Need at least {self.n_clusters} rows to cluster, got {len(X)}")

        X_scaled = self.scaler.fit_transform(X)
        labels   = self.kmeans.fit_predict(X_scaled)
        edr_behavior_data = edr_behavior_data.copy()
        edr_behavior_data["cluster"] = labels

        profiles: Dict[int, Dict] = {}
        for cid in range(self.n_clusters):
            subset = edr_behavior_data[edr_behavior_data["cluster"] == cid]
            profiles[cid] = {
                "name":               self._name_cluster(subset, feature_cols),
                "edrs":               list(subset["edr_name"]),
                "avg_detection_time": float(subset["time_to_detection_ms"].mean())
                                      if "time_to_detection_ms" in subset else 0.0,
                "avg_blocking":       float(subset["blocking_strength"].mean())
                                      if "blocking_strength" in subset else 0.0,
                "characteristics":    self._describe_cluster(subset, feature_cols),
            }
            for edr in list(subset["edr_name"]):
                self._edr_clusters[edr] = cid

        self._profiles = profiles
        return profiles

    # ------------------------------------------------------------------
    # Private helpers
    # ------------------------------------------------------------------

    def _refit(self):
        """Rebuild DataFrame from accumulated observations and re-cluster."""
        df = pd.DataFrame(self._observations).fillna(0)
        feature_cols = [c for c in _NUMERIC_FEATURES if c in df.columns]
        if not feature_cols:
            return

        X = df[feature_cols].values
        X_scaled = self.scaler.fit_transform(X)
        # Use min(n_clusters, n_samples) to avoid crash with few samples
        k = min(self.n_clusters, len(X))
        km = KMeans(n_clusters=k, random_state=42, n_init=10)
        labels = km.fit_predict(X_scaled)
        df["cluster"] = labels

        for cid in range(k):
            subset = df[df["cluster"] == cid]
            self._profiles[cid] = {
                "name":               self._name_cluster(subset, feature_cols),
                "edrs":               list(subset.get("edr_name", pd.Series()).unique()),
                "avg_detection_time": float(subset["time_to_detection_ms"].mean())
                                      if "time_to_detection_ms" in subset.columns else 0.0,
                "avg_blocking":       float(subset["blocking_strength"].mean())
                                      if "blocking_strength" in subset.columns else 0.0,
            }
            for edr in list(subset.get("edr_name", pd.Series()).unique()):
                self._edr_clusters[str(edr)] = cid

        self._df = df

    def _name_cluster(self, cluster_data: pd.DataFrame, feature_cols: List[str]) -> str:
        avg_strength = float(cluster_data["blocking_strength"].mean()) \
            if "blocking_strength" in cluster_data.columns else 0.0
        avg_layers   = float(cluster_data["layer_count"].mean()) \
            if "layer_count"       in cluster_data.columns else 0.0

        if avg_strength > 0.75 and avg_layers > 3.5:
            return "Enterprise Grade (Strong PPL + Multi-Layer)"
        elif avg_strength > 0.55:
            return "Moderate Protection (Driver-Heavy)"
        elif avg_layers < 2.5:
            return "User-Mode Focused (Minimal Kernel Visibility)"
        else:
            return "Basic Detection (Alert-Only)"

    def _describe_cluster(self, cluster_data: pd.DataFrame, _feature_cols: List[str]) -> str:
        parts = []
        if "avg_detection_time" in cluster_data:
            avg_ttd = cluster_data["time_to_detection_ms"].mean() \
                if "time_to_detection_ms" in cluster_data.columns else 0
            parts.append(f"Avg detection latency: {avg_ttd:.0f} ms")
        if "minifilter_present" in cluster_data.columns:
            pct = cluster_data["minifilter_present"].mean() * 100
            parts.append(f"Minifilter coverage: {pct:.0f}%")
        return "; ".join(parts) if parts else "No additional characteristics."

    def _save_observations(self):
        try:
            with open(self._obs_path, "w") as f:
                json.dump(self._observations, f, indent=2)
        except Exception:
            pass

    def _load_observations(self):
        try:
            if self._obs_path.exists():
                with open(self._obs_path) as f:
                    self._observations = json.load(f)
                if len(self._observations) >= self.n_clusters:
                    self._refit()
        except Exception:
            self._observations = []


# ============================================================================
# Quick smoke test
# ============================================================================

if __name__ == "__main__":
    ana = EDRBehaviorAnalyzer(n_clusters=3)

    # Simulate 5 EDR observations
    test_logs = [
        {"edr_name": "CrowdStrike",  "time_to_detection_ms": 120,   "detection_confidence": 0.95,
         "blocking_method": "process_termination", "minifilter_present": True,
         "etw_consumer_active": True, "kernel_callbacks": ["PsSetCreateProcessNotifyRoutine"],
         "network_isolation": True},
        {"edr_name": "Defender",     "time_to_detection_ms": 850,   "detection_confidence": 0.70,
         "blocking_method": "driver_block", "minifilter_present": True,
         "etw_consumer_active": True, "kernel_callbacks": [],
         "network_isolation": False},
        {"edr_name": "CarbonBlack",  "time_to_detection_ms": 6200,  "detection_confidence": 0.40,
         "blocking_method": "alert_only", "minifilter_present": False,
         "etw_consumer_active": False, "kernel_callbacks": [],
         "network_isolation": False},
        {"edr_name": "SentinelOne",  "time_to_detection_ms": 200,   "detection_confidence": 0.90,
         "blocking_method": "process_termination", "minifilter_present": True,
         "etw_consumer_active": True, "kernel_callbacks": ["ObRegisterCallbacks"],
         "network_isolation": True},
        {"edr_name": "ESET",         "time_to_detection_ms": 3100,  "detection_confidence": 0.55,
         "blocking_method": "alert_only", "minifilter_present": True,
         "etw_consumer_active": False, "kernel_callbacks": [],
         "network_isolation": False},
    ]

    for log in test_logs:
        name = log.pop("edr_name")
        features = ana.analyze_response(log)
        ana.accumulate_observation(name, features)

    profiles = ana.get_profiles()
    print("Cluster profiles:")
    for cid, prof in profiles.items():
        print(f"  [{cid}] {prof['name']}: {prof['edrs']}")

    print("\nCrowdStrike cluster:", ana.get_cluster_id("CrowdStrike"))
    print("CarbonBlack vulnerabilities:", ana.get_vulnerabilities("CarbonBlack"))
