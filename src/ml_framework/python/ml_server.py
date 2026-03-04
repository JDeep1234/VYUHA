"""
ML Server - JSON stdio bridge between C++ (MLBridge) and Python ML modules
==========================================================================
Authors: Karthik (ML logic) + Jdeep (integration protocol)

Communication protocol: newline-delimited JSON over stdin/stdout.

Commands handled:
  select_action  -> DQNAgent.select_action()
  train          -> DQNAgent.remember() + replay()
  update_target  -> DQNAgent.update_target_network()
  save_model     -> DQNAgent.save()
  load_model     -> DQNAgent.load()
  cluster        -> EDRBehaviorAnalyzer.cluster_edrs() / analyze_response()
  update_behavior-> EDRBehaviorAnalyzer.accumulate observation
  explain        -> EDRExplainer.explain_failure()
  ping           -> health check

Run standalone for testing:
  python ml_server.py --test
"""

import sys
import json
import os
import traceback
from pathlib import Path

# Add parent dir so we can import sibling modules
_HERE = Path(__file__).parent
sys.path.insert(0, str(_HERE))

from strategy_selector import DQNAgent
from behavior_analyzer import EDRBehaviorAnalyzer
from adaptive_learner  import AdaptiveLearner
from explainable_ai    import EDRExplainer

import numpy as np

# ============================================================================
# Global ML instances (lazy-initialised on first use)
# ============================================================================

_agent: DQNAgent               = None
_behavior: EDRBehaviorAnalyzer = None
_learner: AdaptiveLearner      = None
_explainer: EDRExplainer       = None

_FEATURE_NAMES = [
    # EDR (5 one-hot)
    "edr_defender", "edr_crowdstrike", "edr_carbonblack", "edr_sophos", "edr_other",
    # EDR state (3)
    "edr_version", "edr_process_running", "edr_driver_loaded",
    # System - windows version (4 one-hot)
    "win10", "win11", "server2019", "server2022",
    # System config (8)
    "win_build", "integrity_medium", "integrity_high", "integrity_system",
    "ppl_protection", "driver_sig_enforcement", "secure_boot", "virtualization",
    # Previous attempts (12)
    "last_act_byovd_rtcore", "last_act_byovd_dbutil", "last_act_ppl_bypass",
    "last_act_handle_dup", "last_act_minifilter", "last_act_callback",
    "last_act_syscall", "last_act_wait",
    "last_result_success", "last_result_blocked", "last_result_failed", "last_result_crash",
    # Counters (2)
    "consecutive_failures", "time_since_last_action",
]
assert len(_FEATURE_NAMES) == 30, f"Expected 30 features, got {len(_FEATURE_NAMES)}"

def _get_agent() -> DQNAgent:
    global _agent
    if _agent is None:
        _agent = DQNAgent(state_size=30, action_size=8)
    return _agent

def _get_behavior() -> EDRBehaviorAnalyzer:
    global _behavior
    if _behavior is None:
        _behavior = EDRBehaviorAnalyzer(n_clusters=4)
    return _behavior

def _get_explainer() -> EDRExplainer:
    global _explainer
    if _explainer is None:
        _explainer = EDRExplainer(model=_get_agent(), feature_names=_FEATURE_NAMES)
    return _explainer

def _get_learner() -> AdaptiveLearner:
    global _learner
    if _learner is None:
        _learner = AdaptiveLearner(base_agent=_get_agent())
    return _learner

# ============================================================================
# Command Handlers
# ============================================================================

def handle_select_action(data: dict) -> dict:
    """Select best action for the current system state."""
    state = np.array(data["state"], dtype=np.float32)
    valid_actions = data.get("valid_actions", None)

    agent = _get_agent()
    action_id = agent.select_action(state, valid_actions=valid_actions)
    action_name = agent.get_action_name(action_id)

    # Q-values for all actions (exploit mode, no randomness)
    import torch
    with torch.no_grad():
        state_t = torch.FloatTensor(state).unsqueeze(0)
        q_vals = agent.policy_net(state_t).numpy()[0].tolist()

    return {
        "action": int(action_id),
        "action_name": action_name,
        "q_values": q_vals,
        "epsilon": float(agent.epsilon),
    }


def handle_train(data: dict) -> dict:
    """Store experience and run one replay step."""
    agent = _get_agent()
    state      = np.array(data["state"],      dtype=np.float32)
    action     = int(data["action"])
    reward     = float(data["reward"])
    next_state = np.array(data["next_state"], dtype=np.float32)
    done       = bool(data["done"])

    agent.remember(state, action, reward, next_state, done)
    loss = agent.replay()

    # Also notify AdaptiveLearner for meta-learning
    learner = _get_learner()
    learner.online_update(state, action, reward, next_state, done)

    return {
        "loss":    float(loss) if loss is not None else -1.0,
        "epsilon": float(agent.epsilon),
        "memory_size": len(agent.memory),
    }


def handle_update_target(_data: dict) -> dict:
    _get_agent().update_target_network()
    return {"status": "ok"}


def handle_save_model(data: dict) -> dict:
    path = Path(data["path"])
    path.parent.mkdir(parents=True, exist_ok=True)
    _get_agent().save(path)
    return {"status": "ok", "path": str(path)}


def handle_load_model(data: dict) -> dict:
    path = Path(data["path"])
    if not path.exists():
        return {"status": "error", "message": f"File not found: {path}"}
    agent = _get_agent()
    agent.load(path)
    return {"status": "ok", "epsilon": float(agent.epsilon)}


def handle_cluster(data: dict) -> dict:
    """
    Cluster an EDR based on observed behavioral features.
    features: dict of {feature_name: float_value}
    """
    import pandas as pd

    features = data.get("features", {})
    edr_name = data.get("edr_name", "unknown")

    behavior = _get_behavior()

    # Build single-row dataframe for analysis
    row = behavior.analyze_response(features)
    row["edr_name"] = edr_name

    # Ensure enough rows exist for clustering
    behavior.accumulate_observation(edr_name, row)
    profiles = behavior.get_profiles()

    cluster_id   = behavior.get_cluster_id(edr_name)
    cluster_name = profiles.get(cluster_id, {}).get("name", "Unknown")
    vulns        = behavior.get_vulnerabilities(edr_name)

    return {
        "cluster_id":    cluster_id,
        "cluster_name":  cluster_name,
        "vulnerabilities": vulns,
    }


def handle_update_behavior(data: dict) -> dict:
    features = data.get("features", {})
    edr_name = data.get("edr_name", "unknown")
    behavior = _get_behavior()
    row = behavior.analyze_response(features)
    row["edr_name"] = edr_name
    behavior.accumulate_observation(edr_name, row)
    return {"status": "ok"}


def handle_explain(data: dict) -> dict:
    """Explain why a technique succeeded or failed."""
    technique_id = data.get("technique_id", "")
    edr_name     = data.get("edr_name", "")
    state        = np.array(data.get("state", [0.0] * 30), dtype=np.float32)
    outcome      = data.get("outcome", "unknown")

    explainer = _get_explainer()
    result = explainer.explain_failure(technique_id, edr_name, state, outcome)

    if result is None:
        return {
            "explanation": "Explainer not ready — insufficient data.",
            "top_features": [],
            "narrative": "",
            "valid": False,
        }

    return {
        "explanation":  result.get("explanation", ""),
        "top_features": result.get("top_features", []),
        "narrative":    result.get("narrative", ""),
        "valid":        True,
    }


def handle_transfer_learning(data: dict) -> dict:
    """Transfer knowledge from one EDR model to another."""
    source_edr = data.get("source_edr", "")
    target_edr = data.get("target_edr", "")
    threshold  = float(data.get("similarity_threshold", 0.7))

    learner = _get_learner()
    new_agent = learner.transfer_learning(source_edr, target_edr, threshold)

    if new_agent is None:
        return {"status": "skipped", "reason": "Similarity below threshold or no data"}

    return {"status": "ok", "transferred": True}


def handle_failure_patterns(data: dict) -> dict:
    """Identify recurring failure patterns."""
    history = data.get("execution_history", [])
    learner = _get_learner()
    blacklist = learner.analyze_failure_patterns(history)
    return {"blacklist": blacklist or []}


def handle_ping(_data: dict) -> dict:
    return {"status": "pong", "ready": True}


# ============================================================================
# Dispatch table
# ============================================================================

HANDLERS = {
    "select_action":     handle_select_action,
    "train":             handle_train,
    "update_target":     handle_update_target,
    "save_model":        handle_save_model,
    "load_model":        handle_load_model,
    "cluster":           handle_cluster,
    "update_behavior":   handle_update_behavior,
    "explain":           handle_explain,
    "transfer_learning": handle_transfer_learning,
    "failure_patterns":  handle_failure_patterns,
    "ping":              handle_ping,
}

# ============================================================================
# Main stdio loop
# ============================================================================

def main_loop():
    """Read JSON from stdin, dispatch, write JSON to stdout."""
    # Use binary mode to avoid Windows \r\n issues
    stdin  = sys.stdin.buffer
    stdout = sys.stdout.buffer

    log = open(str(_HERE.parent.parent.parent / "ml_server.log"), "a", encoding="utf-8")

    def write_response(response: dict):
        line = json.dumps(response, separators=(",", ":")) + "\n"
        stdout.write(line.encode("utf-8"))
        stdout.flush()
        log.write(f"OUT: {line}")
        log.flush()

    log.write("=== ml_server.py started ===\n")
    log.flush()

    for raw_line in stdin:
        line = raw_line.decode("utf-8", errors="replace").strip()
        if not line:
            continue

        log.write(f"IN:  {line}\n")
        log.flush()

        try:
            data    = json.loads(line)
            command = data.get("command", "")
            handler = HANDLERS.get(command)

            if handler is None:
                write_response({"error": f"Unknown command: {command}"})
                continue

            result = handler(data)
            write_response(result)

        except Exception as exc:
            tb = traceback.format_exc()
            log.write(f"ERR: {tb}\n")
            log.flush()
            write_response({"error": str(exc), "traceback": tb})

    log.write("=== ml_server.py exiting ===\n")
    log.close()

# ============================================================================
# Self-test
# ============================================================================

def self_test():
    print("=== ML Server self-test ===")

    # 1. Action selection
    state = [0.0] * 30
    state[0] = 1.0   # EDR: Defender running
    state[7] = 1.0   # Kernel driver loaded
    state[14] = 1.0  # High integrity

    resp = handle_select_action({"state": state})
    print(f"[1] select_action -> action={resp['action']} ({resp['action_name']})"
          f"  epsilon={resp['epsilon']:.3f}")

    # 2. Training
    next_state = state.copy()
    next_state[0] = 0.0   # EDR terminated
    train_resp = handle_train({
        "state":      state,
        "action":     resp["action"],
        "reward":     100.0,
        "next_state": next_state,
        "done":       True,
    })
    print(f"[2] train -> loss={train_resp['loss']:.4f}  epsilon={train_resp['epsilon']:.3f}")

    # 3. Save / load cycle
    models_dir = _HERE.parent / "models"
    models_dir.mkdir(exist_ok=True)
    save_path = str(models_dir / "test_self_test.pth")

    save_resp = handle_save_model({"path": save_path})
    print(f"[3] save_model -> {save_resp}")

    load_resp = handle_load_model({"path": save_path})
    print(f"[4] load_model -> {load_resp}")

    # 4. Ping
    ping_resp = handle_ping({})
    print(f"[5] ping -> {ping_resp}")

    print("=== All self-tests passed! ===")


if __name__ == "__main__":
    if "--test" in sys.argv:
        self_test()
    else:
        main_loop()
