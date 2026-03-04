/*
 * ============================================================================
 * EVASION SCORER - Implementation
 * ============================================================================
 * Author: Karthik
 *
 * Tracks and scores evasion success rates per technique and per EDR product.
 * Feeds data into the MLBridge reward calculation and AnalysisReport.
 *
 * Metrics computed:
 *   - Per-technique: success rate, evasion effectiveness, average execution time
 *   - Per-EDR: separate success rates (perEDRScores map)
 *   - Session aggregate: overall framework effectiveness
 * ============================================================================
 */

#include "ml_framework/ml_engine.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace edr {
namespace ml {

// ============================================================================
// EvasionScorer
// ============================================================================

EvasionScorer::EvasionScorer() {}

void EvasionScorer::analyze(const void* /*executionResult*/) {
    // Legacy interface — use recordExecution() instead
    std::cout << "[EvasionScorer] Use recordExecution() with ExploitResult." << std::endl;
}

std::string EvasionScorer::getReport() const {
    if (scores_.empty()) {
        return "Evasion Scoring: No results recorded this session.";
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "Evasion Scoring Report\n";
    oss << std::string(50, '=') << "\n";

    for (const auto& s : scores_) {
        oss << "Technique     : " << s.techniqueId << "\n";
        oss << "Tests run     : " << s.testsRun << "\n";
        oss << "Successful    : " << s.successfulEvasions << "\n";
        oss << "Success rate  : " << (s.successRate * 100.0) << "%\n";
        oss << "Effectiveness : " << (s.evasionEffectiveness * 100.0) << "%\n";
        if (!s.perEDRScores.empty()) {
            oss << "Per-EDR scores:\n";
            for (const auto& [edr, score] : s.perEDRScores) {
                oss << "  " << edr << " : " << (score * 100.0) << "%\n";
            }
        }
        oss << std::string(50, '-') << "\n";
    }

    oss << "Overall effectiveness: "
        << (getOverallEffectiveness() * 100.0) << "%\n";
    return oss.str();
}

void EvasionScorer::reset() {
    scores_.clear();
    perTechniqueCount_.clear();
    perEdrtechniqueSuccess_.clear();
}

// ----------------------------------------------------------------------------
// Record a single execution result and update rolling statistics
// ----------------------------------------------------------------------------

void EvasionScorer::recordExecution(const edr::exploits::ExploitResult& exploit,
                                     bool detected,
                                     const std::string& edrName)
{
    const std::string& id = exploit.techniqueId;
    bool evaded = exploit.success && !detected;

    // Find or create score entry
    auto it = std::find_if(scores_.begin(), scores_.end(),
        [&id](const EvasionScore& s){ return s.techniqueId == id; });

    if (it == scores_.end()) {
        EvasionScore newScore;
        newScore.techniqueId = id;
        newScore.testsRun    = 0;
        newScore.successfulEvasions = 0;
        newScore.successRate        = 0.0;
        newScore.evasionEffectiveness = 0.0;
        scores_.push_back(newScore);
        it = scores_.end() - 1;
    }

    it->testsRun++;
    if (evaded) it->successfulEvasions++;

    it->successRate = static_cast<double>(it->successfulEvasions) / it->testsRun;

    // Effectiveness penalises if detected even when successful
    // (technique is "burned" — EDR now knows about it)
    double detectionPenalty = detected ? 0.20 : 0.0;
    it->evasionEffectiveness = std::max(0.0, it->successRate - detectionPenalty);

    // Per-EDR score
    if (!edrName.empty()) {
        auto& perEdr = it->perEDRScores[edrName];
        // Rolling average using stored count
        auto key = id + "|" + edrName;
        perTechniqueCount_[key]++;
        int  cnt = perTechniqueCount_[key];
        perEdrtechniqueSuccess_[key] += evaded ? 1 : 0;
        perEdr = static_cast<double>(perEdrtechniqueSuccess_[key]) / cnt;
    }
}

// ----------------------------------------------------------------------------
// Calculate score from raw integers (used by MLEngine::analyze fallback)
// ----------------------------------------------------------------------------

EvasionScore EvasionScorer::calculateScore(const std::string& techniqueId,
                                            int totalTests,
                                            int successfulEvasions)
{
    EvasionScore score;
    score.techniqueId       = techniqueId;
    score.testsRun          = totalTests;
    score.successfulEvasions = successfulEvasions;

    if (totalTests > 0) {
        score.successRate = static_cast<double>(successfulEvasions) / totalTests;
        score.evasionEffectiveness = score.successRate;
    } else {
        score.successRate          = 0.0;
        score.evasionEffectiveness = 0.0;
    }
    return score;
}

// ----------------------------------------------------------------------------
// Aggregate metrics
// ----------------------------------------------------------------------------

double EvasionScorer::getOverallEffectiveness() const {
    if (scores_.empty()) return 0.0;
    double total = 0.0;
    for (const auto& s : scores_) total += s.evasionEffectiveness;
    return total / static_cast<double>(scores_.size());
}

std::vector<std::pair<std::string, double>> EvasionScorer::rankTechniques() const {
    std::vector<std::pair<std::string, double>> ranked;
    for (const auto& s : scores_) {
        ranked.emplace_back(s.techniqueId, s.evasionEffectiveness);
    }
    std::sort(ranked.begin(), ranked.end(),
              [](const auto& a, const auto& b){ return a.second > b.second; });
    return ranked;
}

double EvasionScorer::getStealthScore() const {
    // Stealth = fraction of successful executions that went completely undetected.
    int total   = 0;
    int stealth = 0;
    for (const auto& s : scores_) {
        total   += s.testsRun;
        stealth += s.successfulEvasions;
    }
    return (total > 0) ? static_cast<double>(stealth) / total : 0.0;
}

const std::vector<EvasionScore>& EvasionScorer::getScores() const {
    return scores_;
}

} // namespace ml
} // namespace edr
