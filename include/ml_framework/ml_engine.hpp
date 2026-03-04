/*
 * ============================================================================
 * ML FRAMEWORK MODULE - Header
 * ============================================================================
 * Authors: Karthik (ML core) + Jdeep (integration)
 *
 * Machine Learning framework for detection analysis, evasion scoring,
 * MITRE ATT&CK event correlation, and DQN-driven strategy selection.
 *
 * Integration points:
 *   - Receives ExploitResult from Bipin's ExploitManager (via Jdeep's agent)
 *   - Returns AnalysisReport + recommended next action to Jdeep's orchestrator
 * ============================================================================
 */

#ifndef EDR_ML_FRAMEWORK_HPP
#define EDR_ML_FRAMEWORK_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

// Pull in ExploitResult so analyze() is properly typed
#include "exploits/exploit_manager.hpp"
// Pull in Bridge types (SystemState, ActionResponse, etc.)
#include "ml_framework/ml_bridge.hpp"

namespace edr {
namespace ml {

// ============================================================================
// ANALYSIS RESULT STRUCTURES
// ============================================================================

struct DetectionResult {
    std::string techniqueId;
    bool detected;
    double confidenceScore;      // 0.0 - 1.0
    std::string detectionMethod; // Signature, Behavioral, ML, etc.
    std::vector<std::string> matchedPatterns;
    std::string edrResponse;     // Block, Alert, Allow
};

struct EvasionScore {
    std::string techniqueId;
    double successRate;          // 0.0 - 1.0
    double evasionEffectiveness; // 0.0 - 1.0
    int testsRun;
    int successfulEvasions;
    std::map<std::string, double> perEDRScores;  // Score per EDR
};

struct ATTACKMapping {
    std::string techniqueId;
    std::string techniqueName;
    std::string tactic;
    std::vector<std::string> killChainPhases;
    std::vector<std::string> relatedTechniques;
    std::string mitigation;
};

struct CorrelationEvent {
    std::string eventId;
    std::string source;
    std::string timestamp;
    std::vector<ATTACKMapping> mappedTechniques;
    double correlationScore;
};

struct AnalysisReport {
    std::string sessionId;
    std::string timestamp;
    std::vector<DetectionResult> detections;
    std::vector<EvasionScore> evasionScores;
    std::vector<CorrelationEvent> correlations;
    double overallEvasionRate;
    std::string recommendation;
};

// ============================================================================
// BASE ANALYZER CLASS - Inherit for each analysis type
// ============================================================================

class BaseAnalyzer {
public:
    virtual ~BaseAnalyzer() = default;
    
    /**
     * Analyze execution results
     */
    virtual void analyze(const void* executionResult) = 0;
    
    /**
     * Get analysis report
     */
    virtual std::string getReport() const = 0;
    
    /**
     * Reset analyzer state
     */
    virtual void reset() = 0;
};

// ============================================================================
// KARTHIK'S ML IMPLEMENTATIONS - ADD YOUR CODE HERE!
// ============================================================================

class DetectionAnalyzer : public BaseAnalyzer {
public:
    DetectionAnalyzer();
    ~DetectionAnalyzer() override = default;

    void analyze(const void* executionResult) override;
    std::string getReport() const override;
    void reset() override;

    /**
     * Analyse whether a technique was detected, given raw EDR alert strings.
     */
    DetectionResult analyzeDetection(const std::string& techniqueId,
                                     const std::vector<std::string>& edrAlerts);

    /**
     * Full analysis of a typed ExploitResult from Bipin's module.
     * Primary integration point called by MLEngine::analyze().
     */
    DetectionResult analyzeExploitResult(const edr::exploits::ExploitResult& exploit);

    /**
     * Load detection patterns from file (future: JSON/YAML rules).
     */
    bool loadPatterns(const std::string& patternFile);

    /**
     * Train statistical model from labelled (alert, detected) pairs.
     */
    bool trainModel(const std::vector<std::pair<std::string, bool>>& trainingData);

    const std::vector<DetectionResult>& getResults() const;

private:
    std::vector<DetectionResult> results_;
};

class EvasionScorer : public BaseAnalyzer {
public:
    EvasionScorer();
    ~EvasionScorer() override = default;

    void analyze(const void* executionResult) override;
    std::string getReport() const override;
    void reset() override;

    /**
     * Record a single ExploitResult from Bipin's module.
     * Primary integration point called by MLEngine::analyze().
     * @param detected   True if DetectionAnalyzer confirmed EDR detected it.
     * @param edrName    Name of target EDR product.
     */
    void recordExecution(const edr::exploits::ExploitResult& exploit,
                         bool detected,
                         const std::string& edrName = "");

    /**
     * Calculate score from raw counts (fallback / testing).
     */
    EvasionScore calculateScore(const std::string& techniqueId,
                                int totalTests,
                                int successfulEvasions);

    /**
     * Overall session-wide evasion effectiveness (0-1).
     */
    double getOverallEffectiveness() const;

    /**
     * Stealth score: fraction of executions that were fully undetected.
     */
    double getStealthScore() const;

    /**
     * Techniques ranked by effectiveness descending.
     */
    std::vector<std::pair<std::string, double>> rankTechniques() const;

    const std::vector<EvasionScore>& getScores() const;

private:
    std::vector<EvasionScore>      scores_;
    std::map<std::string, int>     perTechniqueCount_;
    std::map<std::string, int>     perEdrtechniqueSuccess_;
};

class EventCorrelator : public BaseAnalyzer {
public:
    EventCorrelator();
    ~EventCorrelator() override = default;

    void analyze(const void* executionResult) override;
    std::string getReport() const override;
    void reset() override;

    /**
     * Correlate a single ExploitResult with ATT&CK framework.
     * Primary integration point called by MLEngine::analyze().
     */
    CorrelationEvent correlateExploitResult(const edr::exploits::ExploitResult& exploit);

    /**
     * Map technique ID to ATT&CK entry.
     */
    ATTACKMapping mapToATTACK(const std::string& techniqueId);

    /**
     * Correlate raw event strings.
     */
    std::vector<CorrelationEvent> correlateEvents(
        const std::vector<std::string>& events);

    /**
     * Load ATT&CK database from JSON file (STIX-lite format).
     */
    bool loadATTACKDatabase(const std::string& dbPath);

    /**
     * Identify ordered kill-chain from a list of event strings.
     */
    std::vector<std::string> identifyAttackChain(
        const std::vector<std::string>& events);

    const std::vector<CorrelationEvent>& getCorrelations() const;

private:
    std::vector<CorrelationEvent>            correlations_;
    std::map<std::string, ATTACKMapping>     attackDatabase_;  ///< runtime ATT&CK DB
};

/*
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                                                                          │
 * │  KARTHIK: ADD MORE ML CLASSES HERE AS NEEDED                            │
 * │                                                                          │
 * │  Examples:                                                               │
 * │    - class BehaviorClassifier : public BaseAnalyzer { ... };            │
 * │    - class AnomalyDetector : public BaseAnalyzer { ... };               │
 * │    - class ThreatPredictor : public BaseAnalyzer { ... };               │
 * │    - class RiskAssessor : public BaseAnalyzer { ... };                  │
 * │                                                                          │
 * └─────────────────────────────────────────────────────────────────────────┘
 */


// ============================================================================
// ML ENGINE - Main ML Framework Coordinator
// ============================================================================

class MLEngine {
public:
    MLEngine();
    ~MLEngine();
    
    /**
     * Initialize ML engine
     */
    bool initialize();
    
    /**
     * Analyze a single exploit execution result.
     * Called by Jdeep's orchestrator after Bipin's ExploitManager returns.
     *
     * @param result    ExploitResult from ExploitManager::execute()
     * @param state     System state snapshot (for DQN reward feedback)
     * @param nextState System state after execution
     */
    AnalysisReport analyze(const edr::exploits::ExploitResult& result,
                           const SystemState& state,
                           const SystemState& nextState);

    /**
     * Ask the DQN to recommend the best next action given system state.
     * Returns action id 0-7 matching the DESIGN.md action space.
     * Called by Jdeep's orchestrator BEFORE exploit execution.
     */
    ActionResponse recommendAction(const SystemState& state,
                                   const std::vector<int>& validActions = {});

    /**
     * Compile session-wide AnalysisReport from all partial results.
     */
    AnalysisReport generateReport() const;

    /**
     * Load ML models from directory (calls Python save/load over bridge).
     */
    bool loadModels(const std::string& modelPath);

    /**
     * Persist ML models to directory.
     */
    bool saveModels(const std::string& modelPath);

    /**
     * Access sub-analyser components directly (used by CLI / OutputHandler).
     */
    DetectionAnalyzer& detectionAnalyzer() { return detectionAnalyzer_; }
    EvasionScorer&     evasionScorer()     { return evasionScorer_;     }
    EventCorrelator&   eventCorrelator()   { return eventCorrelator_;   }
    MLBridge&          bridge()            { return bridge_;            }

private:
    DetectionAnalyzer detectionAnalyzer_;
    EvasionScorer     evasionScorer_;
    EventCorrelator   eventCorrelator_;
    MLBridge          bridge_;

    bool        initialized_;
    std::string sessionId_;
    int         episodeCount_  = 0;
    int         TARGET_SYNC_EVERY = 10;   // sync DQN target network every N episodes

    // Accumulate partial results across a campaign session
    std::vector<DetectionResult>  sessionDetections_;
    std::vector<EvasionScore>     sessionEvasionScores_;
    std::vector<CorrelationEvent> sessionCorrelations_;

    // Persist bridge startup path info
    std::string pythonExe_;
    std::string serverScriptPath_;
};

} // namespace ml
} // namespace edr

#endif // EDR_ML_FRAMEWORK_HPP
