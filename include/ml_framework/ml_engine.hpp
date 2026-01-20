/*
 * ============================================================================
 * ML FRAMEWORK MODULE - Header
 * ============================================================================
 * Author: KARTHIK
 * 
 * Machine Learning framework for detection analysis, evasion scoring,
 * and MITRE ATT&CK event correlation.
 * 
 * KARTHIK: Add your ML implementations here!
 * ============================================================================
 */

#ifndef EDR_ML_FRAMEWORK_HPP
#define EDR_ML_FRAMEWORK_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>

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

/*
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                    DETECTION ANALYZER                                   │
 * │                    Pattern Recognition                                   │
 * │                                                                          │
 * │  KARTHIK: Implement detection pattern analysis here                     │
 * │                                                                          │
 * │  Features to implement:                                                  │
 * │    - Signature-based detection analysis                                 │
 * │    - Behavioral pattern matching                                         │
 * │    - ML model for detection prediction                                   │
 * └─────────────────────────────────────────────────────────────────────────┘
 */
class DetectionAnalyzer : public BaseAnalyzer {
public:
    DetectionAnalyzer();
    ~DetectionAnalyzer() override = default;
    
    void analyze(const void* executionResult) override;
    std::string getReport() const override;
    void reset() override;
    
    /**
     * Analyze single technique detection
     */
    DetectionResult analyzeDetection(const std::string& techniqueId,
                                     const std::vector<std::string>& edrAlerts);
    
    /**
     * Load detection patterns from file
     * KARTHIK: Add ML model loading here
     */
    bool loadPatterns(const std::string& patternFile);
    
    /**
     * Train detection model
     * KARTHIK: Implement ML training here
     */
    bool trainModel(const std::vector<std::pair<std::string, bool>>& trainingData);

private:
    // KARTHIK: Add your private members here
    // std::unique_ptr<MLModel> model_;
    // std::vector<Pattern> patterns_;
    std::vector<DetectionResult> results_;
};

/*
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                    EVASION SCORER                                       │
 * │                    Success Rate Calculation                              │
 * │                                                                          │
 * │  KARTHIK: Implement evasion scoring logic here                          │
 * │                                                                          │
 * │  Features to implement:                                                  │
 * │    - Calculate evasion success rates                                     │
 * │    - Compare across multiple EDR platforms                               │
 * │    - Generate scoring recommendations                                    │
 * └─────────────────────────────────────────────────────────────────────────┘
 */
class EvasionScorer : public BaseAnalyzer {
public:
    EvasionScorer();
    ~EvasionScorer() override = default;
    
    void analyze(const void* executionResult) override;
    std::string getReport() const override;
    void reset() override;
    
    /**
     * Calculate evasion score for technique
     */
    EvasionScore calculateScore(const std::string& techniqueId,
                                 int totalTests,
                                 int successfulEvasions);
    
    /**
     * Get overall framework effectiveness
     */
    double getOverallEffectiveness() const;
    
    /**
     * Compare technique effectiveness
     */
    std::vector<std::pair<std::string, double>> rankTechniques() const;

private:
    // KARTHIK: Add your private members here
    // std::map<std::string, Statistics> techniqueStats_;
    std::vector<EvasionScore> scores_;
};

/*
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                    EVENT CORRELATOR                                     │
 * │                    MITRE ATT&CK Mapping                                 │
 * │                                                                          │
 * │  KARTHIK: Implement event correlation here                              │
 * │                                                                          │
 * │  Features to implement:                                                  │
 * │    - Map events to ATT&CK techniques                                    │
 * │    - Identify attack chains                                              │
 * │    - Correlate related events                                            │
 * └─────────────────────────────────────────────────────────────────────────┘
 */
class EventCorrelator : public BaseAnalyzer {
public:
    EventCorrelator();
    ~EventCorrelator() override = default;
    
    void analyze(const void* executionResult) override;
    std::string getReport() const override;
    void reset() override;
    
    /**
     * Map technique to ATT&CK framework
     */
    ATTACKMapping mapToATTACK(const std::string& techniqueId);
    
    /**
     * Correlate events in timeline
     */
    std::vector<CorrelationEvent> correlateEvents(
        const std::vector<std::string>& events);
    
    /**
     * Load ATT&CK database
     * KARTHIK: Load MITRE data here
     */
    bool loadATTACKDatabase(const std::string& dbPath);
    
    /**
     * Identify attack chain from events
     */
    std::vector<std::string> identifyAttackChain(
        const std::vector<std::string>& events);

private:
    // KARTHIK: Add your private members here
    // std::map<std::string, ATTACKMapping> attackDatabase_;
    std::vector<CorrelationEvent> correlations_;
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
     * Analyze execution results
     * Called by Jdeep's AgentCore after exploit execution
     */
    AnalysisReport analyze(const void* executionResult);
    
    /**
     * Generate comprehensive report
     */
    AnalysisReport generateReport();
    
    /**
     * Load ML models
     * KARTHIK: Implement model loading
     */
    bool loadModels(const std::string& modelPath);
    
    /**
     * Save trained models
     */
    bool saveModels(const std::string& modelPath);
    
    /**
     * Get component references
     */
    DetectionAnalyzer& detectionAnalyzer() { return detectionAnalyzer_; }
    EvasionScorer& evasionScorer() { return evasionScorer_; }
    EventCorrelator& eventCorrelator() { return eventCorrelator_; }

private:
    DetectionAnalyzer detectionAnalyzer_;
    EvasionScorer evasionScorer_;
    EventCorrelator eventCorrelator_;
    
    bool initialized_;
    std::string sessionId_;
};

} // namespace ml
} // namespace edr

#endif // EDR_ML_FRAMEWORK_HPP
