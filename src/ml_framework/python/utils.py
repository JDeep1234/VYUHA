"""
utils.py — ML Framework Helper Utilities
==========================================
Author: Karthik

Shared helper functions used across strategy_selector.py,
behavior_analyzer.py, explainable_ai.py and adaptive_learner.py.

Covers:
  - Feature name definitions (single source of truth)
  - Action space definitions
  - State vector builders
  - Reward calculation helpers
  - Logging helpers
"""

import os
import json
import logging
import numpy as np
from datetime import datetime
from typing import Any, Dict, List, Optional, Tuple

# ===========================================================================
# Logging
# ===========================================================================

def get_logger(name: str, log_file: Optional[str] = None) -> logging.Logger:
    """Return a formatted logger; optionally write to file as well."""
    logger = logging.getLogger(name)
    if logger.handlers:
        return logger  # already configured

    logger.setLevel(logging.DEBUG)
    fmt = logging.Formatter("[%(asctime)s] [%(name)s] %(levelname)s  %(message)s",
                             "%Y-%m-%d %H:%M:%S")

    ch = logging.StreamHandler()
    ch.setFormatter(fmt)
    logger.addHandler(ch)

    if log_file:
        os.makedirs(os.path.dirname(log_file), exist_ok=True)
        fh = logging.FileHandler(log_file)
        fh.setFormatter(fmt)
        logger.addHandler(fh)

    return logger


# ===========================================================================
# Action Space  (DESIGN.md Section 1 — Action Space)
# ===========================================================================

ACTIONS: List[str] = [
    "BYOVD_RTCore",       # 0 – Load RTCore64.sys vulnerable driver
    "BYOVD_DBUtil",       # 1 – Load DBUtil driver
    "PPL_Bypass",         # 2 – Attempt PPL elevation/bypass
    "Handle_Duplication", # 3 – Steal handle from privileged process
    "Minifilter_Unload",  # 4 – Unload EDR filesystem minifilter
    "Callback_Removal",   # 5 – Remove kernel callbacks
    "Direct_Syscall",     # 6 – NtTerminateProcess via direct syscall
    "Wait_Observe",       # 7 – No-op / reconnaissance
]

ACTION_TO_MITRE: Dict[int, str] = {
    0: "T1068",
    1: "T1562.001",
    2: "T1134",
    3: "T1055.001",
    4: "T1574.002",
    5: "T1562.006",
    6: "T1055",
    7: "T1218.002",
}

MITRE_TO_ACTION: Dict[str, int] = {v: k for k, v in ACTION_TO_MITRE.items()}

ACTION_SIZE = len(ACTIONS)


# ===========================================================================
# State Space  (DESIGN.md Section 1 — State Space, 30 features)
# ===========================================================================

# The order here MUST match SystemState::toVector() in ml_bridge.cpp
FEATURE_NAMES: List[str] = [
    # EDR product one-hot  [0-4]
    "edr_defender",
    "edr_crowdstrike",
    "edr_carbonblack",
    "edr_sentinelone",
    "edr_other",
    # EDR runtime state    [5-7]
    "edr_process_running",
    "edr_driver_loaded",
    "edr_crashed",
    # Windows version one-hot [8-11]
    "win10",
    "win11",
    "win_server",
    "win_other",
    # System config booleans [12-19]
    "ppl_protection",
    "driver_sig_enforcement",
    "secure_boot",
    "virtualization_enabled",
    "antimalware_light",
    "integrity_high",
    "integrity_system",
    "integrity_medium",
    # Last action one-hot [20-27]
    "last_action_0",
    "last_action_1",
    "last_action_2",
    "last_action_3",
    "last_action_4",
    "last_action_5",
    "last_action_6",
    "last_action_7",
    # Execution counters [28-29]
    "last_result",          # 0=success 1=blocked 2=failed 3=crash -1=none → normalised
    "consecutive_failures", # normalised 0-1
]

STATE_SIZE = len(FEATURE_NAMES)  # 30


# ===========================================================================
# Known EDR product names → one-hot index
# ===========================================================================

EDR_PRODUCTS: Dict[str, int] = {
    "defender":    0,
    "crowdstrike": 1,
    "carbonblack": 2,
    "sentinelone": 3,
}

EDR_NAMES: List[str] = ["Defender", "CrowdStrike", "CarbonBlack", "SentinelOne", "Other"]


def edr_name_to_index(name: str) -> int:
    """Return one-hot index for EDR product name (case-insensitive)."""
    return EDR_PRODUCTS.get(name.lower(), 4)  # 4 = Other


# ===========================================================================
# Reward Calculation  (DESIGN.md reward formula)
# ===========================================================================

class Outcome:
    EDR_TERMINATED      = "edr_terminated"
    TELEMETRY_SILENCED  = "telemetry_silenced"
    PARTIAL_SUCCESS     = "partial_success"
    NO_CHANGE           = "no_change"
    DETECTED_ALERT      = "detected_alert"
    ACTION_BLOCKED      = "action_blocked"
    SYSTEM_CRASH        = "system_crash"


BASE_REWARDS: Dict[str, float] = {
    Outcome.EDR_TERMINATED:     100.0,
    Outcome.TELEMETRY_SILENCED:  75.0,
    Outcome.PARTIAL_SUCCESS:     50.0,
    Outcome.NO_CHANGE:            0.0,
    Outcome.DETECTED_ALERT:     -50.0,
    Outcome.ACTION_BLOCKED:     -75.0,
    Outcome.SYSTEM_CRASH:      -100.0,
}


def classify_outcome(success: bool, detected: bool, output: str) -> str:
    """Map raw exploit result fields to an Outcome string."""
    out_lower = output.lower() if output else ""
    if "crash" in out_lower or "bsod" in out_lower:
        return Outcome.SYSTEM_CRASH
    if success and not detected:
        return Outcome.EDR_TERMINATED
    if success and detected:
        return Outcome.PARTIAL_SUCCESS
    if not success and detected:
        if "blocked" in out_lower or "quarantine" in out_lower:
            return Outcome.ACTION_BLOCKED
        return Outcome.DETECTED_ALERT
    return Outcome.NO_CHANGE


def calculate_reward(outcome: str,
                     stealth_score: float = 0.5,
                     detected: bool = False) -> float:
    """
    DESIGN.md reward formula:
      base + stealth_bonus (if stealth_score > 0.8) + technique_burn (if detected)
    """
    base = BASE_REWARDS.get(outcome, 0.0)
    stealth_bonus  = 20.0 if stealth_score > 0.8 else 0.0
    technique_burn = -30.0 if detected else 0.0
    return base + stealth_bonus + technique_burn


# ===========================================================================
# State vector helpers
# ===========================================================================

def build_state_vector(
    edr_name:         str  = "other",
    edr_running:      bool = True,
    edr_driver:       bool = True,
    edr_crashed:      bool = False,
    win_build:        int  = 22000,
    ppl:              bool = True,
    dse:              bool = True,
    secure_boot:      bool = True,
    virt:             bool = False,
    elam:             bool = True,
    integrity:        int  = 1,        # 0=medium 1=high 2=system
    last_action:      int  = -1,       # -1 = none
    last_result:      int  = -1,       # 0=success 1=blocked 2=failed 3=crash
    consec_fails:     int  = 0,
) -> List[float]:
    """
    Build a 30-element float vector that matches FEATURE_NAMES ordering.
    This mirrors SystemState::fromContext() in ml_bridge.cpp.
    """
    vec = [0.0] * STATE_SIZE

    # EDR one-hot [0-4]
    idx = edr_name_to_index(edr_name)
    vec[idx] = 1.0

    # EDR runtime [5-7]
    vec[5] = 1.0 if edr_running  else 0.0
    vec[6] = 1.0 if edr_driver   else 0.0
    vec[7] = 1.0 if edr_crashed  else 0.0

    # Windows version one-hot [8-11]
    if win_build >= 22000:
        vec[9] = 1.0   # Win11
    elif win_build >= 10000:
        vec[8] = 1.0   # Win10
    else:
        vec[10] = 1.0  # Server

    # System config booleans [12-19]
    vec[12] = 1.0 if ppl        else 0.0
    vec[13] = 1.0 if dse        else 0.0
    vec[14] = 1.0 if secure_boot else 0.0
    vec[15] = 1.0 if virt       else 0.0
    vec[16] = 1.0 if elam       else 0.0
    # integrity one-hot (medium=19, high=17, system=18)
    if integrity == 2:
        vec[18] = 1.0
    elif integrity == 1:
        vec[17] = 1.0
    else:
        vec[19] = 1.0

    # Last action one-hot [20-27]
    if 0 <= last_action < 8:
        vec[20 + last_action] = 1.0

    # Execution counters [28-29]
    # last_result normalised: -1→-0.5, 0→1.0, 1→0.0, 2→-0.25, 3→-1.0
    result_map = {-1: -0.5, 0: 1.0, 1: 0.0, 2: -0.25, 3: -1.0}
    vec[28] = result_map.get(last_result, -0.5)
    vec[29] = min(1.0, consec_fails / 10.0)

    return vec


def state_dict_to_vector(state: Dict[str, Any]) -> List[float]:
    """Convert a flat dict {feature_name: value} to the 30-float vector."""
    return [float(state.get(name, 0.0)) for name in FEATURE_NAMES]


# ===========================================================================
# Serialisation helpers
# ===========================================================================

def save_json(obj: Any, path: str) -> None:
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "w") as f:
        json.dump(obj, f, indent=2, default=str)


def load_json(path: str, default: Any = None) -> Any:
    if not os.path.exists(path):
        return default
    with open(path) as f:
        return json.load(f)


# ===========================================================================
# Evaluation helpers
# ===========================================================================

def compute_success_rate(history: List[Dict]) -> float:
    """Fraction of episodes with positive reward."""
    if not history:
        return 0.0
    successes = sum(1 for h in history if h.get("reward", 0) > 0)
    return successes / len(history)


def rolling_average(values: List[float], window: int = 20) -> List[float]:
    """Compute rolling mean with the given window size."""
    if not values:
        return []
    out = []
    for i in range(len(values)):
        start = max(0, i - window + 1)
        out.append(float(np.mean(values[start:i+1])))
    return out


def print_training_summary(episode: int, reward: float,
                            epsilon: float, success_rate: float) -> None:
    bar  = "█" * int(success_rate * 20)
    bar += "░" * (20 - len(bar))
    print(f"  Ep {episode:5d} | reward={reward:+7.1f} | ε={epsilon:.3f} "
          f"| success [{bar}] {success_rate*100:.1f}%")


# ===========================================================================
# Timestamp
# ===========================================================================

def now_str() -> str:
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


# ===========================================================================
# Quick self-test
# ===========================================================================

if __name__ == "__main__":
    print("=== utils.py self-test ===")
    print(f"  STATE_SIZE  = {STATE_SIZE}")
    print(f"  ACTION_SIZE = {ACTION_SIZE}")
    vec = build_state_vector(edr_name="crowdstrike", ppl=True, last_action=2,
                             last_result=1, consec_fails=3)
    assert len(vec) == STATE_SIZE, "Vector length mismatch!"
    print(f"  build_state_vector() → {len(vec)} floats ✓")

    r = calculate_reward(Outcome.EDR_TERMINATED, stealth_score=0.9)
    assert r == 120.0, f"Expected 120.0 got {r}"
    print(f"  calculate_reward(EDR_TERMINATED, stealth=0.9) = {r} ✓")

    outcome = classify_outcome(success=True, detected=False, output="process terminated")
    assert outcome == Outcome.EDR_TERMINATED
    print(f"  classify_outcome() = {outcome} ✓")

    print("All checks passed.")
