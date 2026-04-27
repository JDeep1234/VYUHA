/*
 * ============================================================================
 * ML ENGINE - Implementation
 * ============================================================================
 * Authors: Karthik (ML models) + Jdeep (C++ integration)
 *
 * Central coordinator that:
 *   1. Receives ExploitResult from Bipin's ExploitManager (via Jdeep's agent)
 *   2. Runs detection / evasion / correlation analysis (Karthik's analyzers)
 *   3. Sends a typed DQN reward signal back over the Python bridge
 *   4. Returns a fully populated AnalysisReport for Jdeep's OutputHandler
 * ============================================================================
 */

#include "ml_framework/ml_engine.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace edr {
namespace ml {

// ============================================================================
// ML ENGINE
// ============================================================================

MLEngine::MLEngine()
    : initialized_(false), episodeCount_(0) {
    std::cout << "[MLEngine] Initialized." << std::endl;
}

MLEngine::~MLEngine() = default;

bool MLEngine::initialize() {
    std::cout << "[MLEngine] Initializing ML components..." << std::endl;

    // Start Python ML server bridge
    // Script path resolved relative to common build layouts; can be overridden
    // via loadModels() or by setting serverScriptPath_ before calling initialize().
    pythonExe_       = "python";
    serverScriptPath_ = "";  // auto-resolve inside MLBridge::start()

    if (!bridge_.start(pythonExe_, serverScriptPath_)) {
        std::cerr << "[MLEngine] Warning: Python bridge failed to start. "
                     "Running in C++-only mode (no DQN action selection)." << std::endl;
        // Continue anyway — analyzer components still work
    }

    initialized_ = true;

    // Generate session ID
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << "ml_session_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    sessionId_ = oss.str();

    std::cout << "[MLEngine] Session: " << sessionId_ << std::endl;
    return true;
}

// ============================================================================
// Core analysis — called by Jdeep's orchestrator after each exploit execution
// ============================================================================

AnalysisReport MLEngine::analyze(
    const edr::exploits::ExploitResult& exploit,
    const SystemState& state,
    const SystemState& nextState)
{
    AnalysisReport report;
    report.sessionId = sessionId_;

    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    report.timestamp = oss.str();

    std::cout << "[MLEngine] Analysing technique: " << exploit.techniqueId << std::endl;

    // -----------------------------------------------------------------------
    // 1. Detection Analysis (Karthik's DetectionAnalyzer)
    // -----------------------------------------------------------------------
    DetectionResult detection = detectionAnalyzer_.analyzeExploitResult(exploit);
    report.detections.push_back(detection);
    sessionDetections_.push_back(detection);

    // -----------------------------------------------------------------------
    // 2. Evasion Scoring (Karthik's EvasionScorer)
    // -----------------------------------------------------------------------
    // EDR name: allow callers to encode it as "edr:<name>" in the artifacts vector
    // (e.g. artifacts.push_back("edr:CrowdStrike")).
    // Falls back to empty string if not found.
    std::string edrName;
    const std::string EDR_PREFIX = "edr:";
    for (const auto& art : exploit.artifacts) {
        if (art.size() > EDR_PREFIX.size() &&
            art.substr(0, EDR_PREFIX.size()) == EDR_PREFIX) {
            edrName = art.substr(EDR_PREFIX.size());
            break;
        }
    }

    evasionScorer_.recordExecution(exploit, detection.detected, edrName);

    // Build a partial EvasionScore for this report entry
    EvasionScore score;
    score.techniqueId            = exploit.techniqueId;
    score.testsRun               = 1;
    score.successfulEvasions     = (exploit.success && !detection.detected) ? 1 : 0;
    score.successRate            = static_cast<double>(score.successfulEvasions);
    score.evasionEffectiveness   = score.successRate;
    report.evasionScores.push_back(score);
    sessionEvasionScores_.push_back(score);

    // -----------------------------------------------------------------------
    // 3. Event Correlation (Karthik's EventCorrelator)
    // -----------------------------------------------------------------------
    CorrelationEvent correlation = eventCorrelator_.correlateExploitResult(exploit);
    report.correlations.push_back(correlation);
    sessionCorrelations_.push_back(correlation);

    // -----------------------------------------------------------------------
    // 4. DQN Reward Feedback (Python Bridge → AdaptiveLearner)
    // -----------------------------------------------------------------------
    if (bridge_.isRunning()) {
        float stealthScore = static_cast<float>(evasionScorer_.getStealthScore());
        float rewardVal    = reward::calculate(
            exploit.success, detection.detected, exploit.output, stealthScore);

        bool done = exploit.success;  // episode ends on full success

        auto trainResp = bridge_.train(state, -1 /*action filled by orchestrator*/,
                                       rewardVal, nextState, done);
        if (trainResp.valid) {
            std::cout << "[MLEngine] DQN trained. Loss=" << trainResp.loss
                      << " Epsilon=" << trainResp.epsilon << std::endl;
        }

        // Sync target network every TARGET_SYNC_EVERY episodes
        ++episodeCount_;
        if (episodeCount_ % TARGET_SYNC_EVERY == 0) {
            bridge_.updateTargetNetwork();
            std::cout << "[MLEngine] DQN target network synced (episode "
                      << episodeCount_ << ")." << std::endl;
        }

        // Update behavior profile in Python (for clustering)
        if (!edrName.empty()) {
            std::map<std::string, float> features;
            features["detection_confidence"] = static_cast<float>(detection.confidenceScore);
            features["blocking_strength"]    = detection.edrResponse == "Block" ? 1.f :
                                               detection.edrResponse == "Alert" ? 0.5f : 0.f;
            features["time_to_detection_ms"] = 500.f;  // placeholder; filled by telemetry
            bridge_.updateBehavior(edrName, features);
        }
    }

    // -----------------------------------------------------------------------
    // 5. Report aggregates
    // -----------------------------------------------------------------------
    report.overallEvasionRate = evasionScorer_.getOverallEffectiveness();

    // Recommendation: if bridge is up, request explanation
    if (bridge_.isRunning() && !edrName.empty()) {
        std::string outcome = detection.detected ?
            (exploit.success ? "partial_success" : "blocked") :
            (exploit.success ? "success" : "failed");
        auto explain = bridge_.explain(exploit.techniqueId, edrName, state, outcome);
        if (explain.valid && !explain.explanation.empty()) {
            report.recommendation = explain.explanation;
        }
    }

    if (report.recommendation.empty()) {
        auto ranked = evasionScorer_.rankTechniques();
        if (!ranked.empty()) {
            report.recommendation = "Top technique by evasion effectiveness: "
                + ranked[0].first + " (" + std::to_string(ranked[0].second * 100.0) + "%)";
        }
    }

    std::cout << "[MLEngine] Analysis complete for " << exploit.techniqueId << "." << std::endl;
    return report;
}

// ============================================================================
// DQN action recommendation — called BEFORE exploit execution
// ============================================================================

ActionResponse MLEngine::recommendAction(const SystemState& state,
                                          const std::vector<int>& validActions) {
    if (!bridge_.isRunning()) {
        // Bridge offline: return default no-op action
        ActionResponse fallback;
        fallback.actionId   = -1;
        fallback.actionName = "Offline";
        fallback.valid      = false;
        return fallback;
    }
    return bridge_.selectAction(state, validActions);
}

// ============================================================================
// Session-wide report
// ============================================================================

AnalysisReport MLEngine::generateReport() const {
    AnalysisReport report;
    report.sessionId  = sessionId_;
    report.detections = sessionDetections_;

    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    report.timestamp = oss.str();

    // Merge session-level evasion scores (aggregate by technique)
    std::map<std::string, EvasionScore> merged;
    for (const auto& s : sessionEvasionScores_) {
        auto& m = merged[s.techniqueId];
        m.techniqueId         = s.techniqueId;
        m.testsRun           += s.testsRun;
        m.successfulEvasions += s.successfulEvasions;
    }
    for (auto& [id, m] : merged) {
        if (m.testsRun > 0) {
            m.successRate          = static_cast<double>(m.successfulEvasions) / m.testsRun;
            m.evasionEffectiveness = m.successRate;
        }
        report.evasionScores.push_back(m);
    }

    report.correlations      = sessionCorrelations_;
    report.overallEvasionRate = evasionScorer_.getOverallEffectiveness();

    // Ranked summary
    auto ranked = evasionScorer_.rankTechniques();
    if (!ranked.empty()) {
        std::ostringstream rec;
        rec << "Session summary — Overall evasion: "
            << std::fixed << std::setprecision(1)
            << (report.overallEvasionRate * 100.0) << "%. ";
        rec << "Best technique: " << ranked[0].first
            << " (" << (ranked[0].second * 100.0) << "%).";
        report.recommendation = rec.str();
    }

    return report;
}

// ============================================================================
// Model persistence
// ============================================================================

bool MLEngine::loadModels(const std::string& modelPath) {
    std::cout << "[MLEngine] Loading ML models from: " << modelPath << std::endl;
    if (!bridge_.isRunning()) return false;
    std::string checkpoint = modelPath + "/dqn_agent.pth";
    return bridge_.loadModel(checkpoint);
}

bool MLEngine::saveModels(const std::string& modelPath) {
    std::cout << "[MLEngine] Saving ML models to: " << modelPath << std::endl;
    if (!bridge_.isRunning()) return false;
    // Create directory if needed
    std::filesystem::create_directories(modelPath);
    std::string checkpoint = modelPath + "/dqn_agent.pth";
    return bridge_.saveModel(checkpoint);
}

} // namespace ml
} // namespace edr
