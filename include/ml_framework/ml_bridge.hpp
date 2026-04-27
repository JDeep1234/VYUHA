/*
 * ============================================================================
 * ML BRIDGE - C++ <-> Python Subprocess Bridge
 * ============================================================================
 * Authors: Karthik (ML) + Jdeep (Integration)
 *
 * Communicates with ml_server.py over stdin/stdout using JSON messages.
 * This avoids pybind11 build complexity while keeping full Python ML power.
 *
 * Protocol (newline-delimited JSON):
 *   C++ -> Python: {"command": "select_action", "state": [...30 floats...]}
 *   Python -> C++: {"action": 3, "action_name": "Handle_Duplication", "q_values": [...]}
 *
 *   C++ -> Python: {"command": "train", "state": [...], "action": 3,
 *                   "reward": 100.0, "next_state": [...], "done": true}
 *   Python -> C++: {"loss": 0.034}
 *
 *   C++ -> Python: {"command": "save", "path": "models/checkpoint.pth"}
 *   Python -> C++: {"status": "ok"}
 *
 *   C++ -> Python: {"command": "load", "path": "models/checkpoint.pth"}
 *   Python -> C++: {"status": "ok", "epsilon": 0.42}
 *
 *   C++ -> Python: {"command": "cluster", "features": {...}}
 *   Python -> C++: {"cluster_id": 2, "cluster_name": "Enterprise Grade", "vulnerabilities": [...]}
 *
 *   C++ -> Python: {"command": "explain", "technique_id": "T1068",
 *                   "edr_name": "CrowdStrike", "state": [...], "outcome": "blocked"}
 *   Python -> C++: {"explanation": "...", "top_features": [...], "narrative": "..."}
 * ============================================================================
 */

#ifndef EDR_ML_BRIDGE_HPP
#define EDR_ML_BRIDGE_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdio>

namespace edr {
namespace ml {

// ============================================================================
// SYSTEM STATE - 30-feature vector fed into DQN
// ============================================================================

struct SystemState {
    // EDR Information (indices 0-3)
    float edrProduct[5]       = {};   // one-hot: Defender/CS/CB/Sophos/Other
    float edrVersion          = 0.f;  // normalised version
    float edrProcessRunning   = 0.f;
    float edrDriverLoaded     = 0.f;

    // System Configuration (indices 8-17)
    float windowsVersion[4]   = {};   // one-hot: Win10/Win11/Server2019/Server2022
    float windowsBuild        = 0.f;  // normalised (0-1)
    float integrityLevel[3]   = {};   // one-hot: Medium/High/System
    float pplProtection       = 0.f;
    float driverSigEnforcement= 0.f;
    float secureBoot          = 0.f;
    float virtualization      = 0.f;
    float antimalwareLight    = 0.f;

    // Previous Attempts (indices 18-29)
    float lastAction[4]       = {};   // one-hot: last technique tried
    float lastActionResult[4] = {};   // one-hot: Success/Blocked/Failed/Crash
    float consecutiveFailures = 0.f;  // normalised (0-1)
    float timeSinceLastAction = 0.f;  // normalised (0-1)

    /**
     * Flatten to 30-element float vector for the DQN.
     */
    std::vector<float> toVector() const;

    /**
     * Build state from integration data (called by Jdeep's agent).
     */
    static SystemState fromContext(
        const std::string& edrName,
        const std::string& edrVersion,
        bool edrRunning,
        bool driverLoaded,
        int  windowsBuild,
        int  intLevel,          // 0=Medium, 1=High, 2=System
        bool ppl,
        bool dse,
        bool secureBoot,
        bool virt,
        bool elam,
        int  lastActionId,      // -1 = none
        int  lastResultId,      // 0=success 1=blocked 2=failed 3=crash, -1=none
        int  consecutiveFails,
        double secondsSinceLast
    );
};

// ============================================================================
// BRIDGE RESPONSE TYPES
// ============================================================================

struct ActionResponse {
    int    actionId    = 0;
    std::string actionName;
    std::vector<float> qValues;   // Q-value for each of 8 actions
    bool   valid       = false;
};

struct TrainResponse {
    float  loss        = 0.f;
    float  epsilon     = 1.f;
    bool   valid       = false;
};

struct ExplainResponse {
    std::string explanation;
    std::vector<std::pair<std::string, float>> topFeatures;  // (name, importance)
    std::string narrative;
    bool        valid = false;
};

struct ClusterResponse {
    int         clusterId    = -1;
    std::string clusterName;
    std::vector<std::string> vulnerabilities;
    bool        valid        = false;
};

// ============================================================================
// ML BRIDGE CLASS
// ============================================================================

class MLBridge {
public:
    MLBridge();
    ~MLBridge();

    /**
     * Launch the Python ml_server.py subprocess.
     * @param pythonPath  Path to python executable (default: "python")
     * @param scriptPath  Path to ml_server.py
     */
    bool start(const std::string& pythonPath = "python",
               const std::string& scriptPath = "");

    /**
     * Shut down the Python process.
     */
    void stop();

    bool isRunning() const { return running_; }

    // -----------------------------------------------------------------------
    // DQN interface (StrategySelector)
    // -----------------------------------------------------------------------

    /**
     * Ask the DQN to select an action given the current system state.
     * Called by Jdeep's orchestrator before each exploit execution.
     */
    ActionResponse selectAction(const SystemState& state,
                                const std::vector<int>& validActions = {});

    /**
     * Send execution outcome back to DQN for online training.
     * Called by Jdeep's orchestrator after Bipin's exploit finishes.
     */
    TrainResponse  train(const SystemState& state,
                         int action,
                         float reward,
                         const SystemState& nextState,
                         bool done);

    /**
     * Sync target network (call every 10 episodes).
     */
    bool updateTargetNetwork();

    /**
     * Persist model checkpoint.
     */
    bool saveModel(const std::string& path);

    /**
     * Load model checkpoint.
     */
    bool loadModel(const std::string& path);

    // -----------------------------------------------------------------------
    // BehaviorAnalyzer interface
    // -----------------------------------------------------------------------

    /**
     * Cluster an EDR by its observed response features.
     * @param features  Map of feature_name -> value from execution logs.
     */
    ClusterResponse clusterEDR(const std::map<std::string, float>& features,
                               const std::string& edrName);

    /**
     * Update behavior database with new observation.
     */
    bool updateBehavior(const std::string& edrName,
                        const std::map<std::string, float>& features);

    // -----------------------------------------------------------------------
    // ExplainableAI interface
    // -----------------------------------------------------------------------

    /**
     * Explain why a technique succeeded or failed.
     * Called by MLEngine::generateReport() to enrich the AnalysisReport.
     */
    ExplainResponse explain(const std::string& techniqueId,
                            const std::string& edrName,
                            const SystemState& state,
                            const std::string& outcome);

private:
    bool   running_  = false;
    FILE*  toChild_  = nullptr;   // stdin of Python process
    FILE*  fromChild_= nullptr;   // stdout of Python process

#ifdef _WIN32
    void*  processHandle_ = nullptr;
#else
    pid_t  childPid_ = -1;
#endif

    // Send a JSON string (with newline) and receive the JSON response.
    std::string sendReceive(const std::string& jsonMsg);

    // Simple JSON helpers (no external library required).
    static std::string buildJson(const std::map<std::string, std::string>& kv);
    static std::string vectorToJsonArray(const std::vector<float>& v);
    static std::string mapToJsonObject(const std::map<std::string, float>& m);

    // Parse key from flat JSON response.
    static std::string  jsonGetString(const std::string& json, const std::string& key);
    static float        jsonGetFloat (const std::string& json, const std::string& key);
    static int          jsonGetInt   (const std::string& json, const std::string& key);
};

// ============================================================================
// REWARD CALCULATOR - maps ExploitResult to DQN reward
// ============================================================================

namespace reward {

// Outcome identifiers (matches DESIGN.md reward function)
enum class Outcome {
    EDR_TERMINATED   = 0,
    TELEMETRY_SILENCED,
    PARTIAL_SUCCESS,
    NO_CHANGE,
    DETECTED_ALERT,
    ACTION_BLOCKED,
    SYSTEM_CRASH
};

/**
 * Calculate reward for the DQN based on exploit execution result.
 * Integrates with Bipin's ExploitResult fields.
 *
 * @param success        ExploitResult::success
 * @param detected       ExploitResult::detected
 * @param output         ExploitResult::output (checked for keywords)
 * @param stealthScore   0.0-1.0 from EvasionScorer
 */
float calculate(bool success, bool detected,
                const std::string& output,
                float stealthScore = 0.5f);

/**
 * Map raw ExploitResult to Outcome enum.
 */
Outcome classifyOutcome(bool success, bool detected, const std::string& output);

} // namespace reward

} // namespace ml
} // namespace edr

#endif // EDR_ML_BRIDGE_HPP
