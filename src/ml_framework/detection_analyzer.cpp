/*
 * ============================================================================
 * DETECTION ANALYZER - Implementation
 * ============================================================================
 * Author: Karthik
 *
 * Analyses ExploitResult data from Bipin's exploit module (via Jdeep's agent)
 * to determine whether the EDR detected the technique and with what confidence.
 *
 * Detection logic:
 *   1. Alert keyword matching against known EDR alert patterns
 *   2. Execution outcome analysis (success=True but EDR active → likely detected)
 *   3. Confidence scoring based on alert count and keyword weight
 * ============================================================================
 */

#include "ml_framework/ml_engine.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <map>

namespace edr {
namespace ml {

// ============================================================================
// Alert keyword → detection weight table
// Keywords are matched case-insensitively against ExploitResult::output and
// ExploitResult::edrAlerts strings.
// ============================================================================

static const std::map<std::string, double> ALERT_KEYWORDS = {
    // Generic detection phrases
    {"detected",           0.90},
    {"blocked",            0.95},
    {"quarantine",         0.85},
    {"threat",             0.80},
    {"malware",            0.80},
    {"suspicious",         0.65},
    {"alert",              0.70},
    {"violation",          0.75},
    {"access denied",      0.85},
    {"terminated",         0.80},  // process killed by EDR
    // BYOVD / driver specific
    {"driver blocked",     0.95},
    {"unsigned driver",    0.90},
    {"dse",                0.85},
    {"kernel",             0.50},
    // Process injection
    {"process injection",  0.90},
    {"injected",           0.85},
    {"shellcode",          0.85},
    // Privilege escalation
    {"privilege",          0.60},
    {"elevation",          0.55},
    {"impersonation",      0.60},
    // EDR self-protection
    {"ppl",                0.75},
    {"protected process",  0.80},
    {"tamper",             0.80},
};

// Detection method label table (techniqueId → primary method)
static const std::map<std::string, std::string> TECHNIQUE_METHODS = {
    {"T1068",      "Kernel Exploit (BYOVD)"},
    {"T1055",      "Process Injection"},
    {"T1055.001",  "DLL Injection"},
    {"T1055.012",  "Process Hollowing"},
    {"T1218.002",  "Signed Binary Proxy Execution"},
    {"T1574.002",  "DLL Side-Loading"},
    {"T1562.001",  "Impair Defenses: Disable/Modify Tools"},
    {"T1562.006",  "Impair Defenses: Indicator Blocking"},
    {"T1134",      "Access Token Manipulation"},
};


// ============================================================================
// DetectionAnalyzer
// ============================================================================

DetectionAnalyzer::DetectionAnalyzer() {
    // Nothing special — patterns are compiled inline for portability
}

void DetectionAnalyzer::analyze(const void* /*executionResult*/) {
    // Legacy void* interface — not used; use analyzeDetection() instead
    std::cout << "[DetectionAnalyzer] Use analyzeDetection() with ExploitResult." << std::endl;
}

std::string DetectionAnalyzer::getReport() const {
    if (results_.empty()) {
        return "Detection Analysis: No results recorded this session.";
    }

    std::ostringstream oss;
    oss << "Detection Analysis Report\n";
    oss << std::string(50, '=') << "\n";

    int detected = 0;
    for (const auto& r : results_) {
        oss << "Technique : " << r.techniqueId << "\n";
        oss << "Detected  : " << (r.detected ? "YES" : "NO") << "\n";
        oss << "Confidence: " << r.confidenceScore << "\n";
        oss << "Method    : " << r.detectionMethod << "\n";
        if (!r.matchedPatterns.empty()) {
            oss << "Patterns  : ";
            for (const auto& p : r.matchedPatterns) oss << "[" << p << "] ";
            oss << "\n";
        }
        oss << "EDR Response: " << r.edrResponse << "\n";
        oss << std::string(50, '-') << "\n";
        if (r.detected) ++detected;
    }

    oss << "Total: " << results_.size() << " techniques, "
        << detected << " detected.\n";
    return oss.str();
}

void DetectionAnalyzer::reset() {
    results_.clear();
}

// ----------------------------------------------------------------------------
// Core analysis: given a techniqueId and raw EDR alert strings,
// determine whether the technique was detected.
// ----------------------------------------------------------------------------

DetectionResult DetectionAnalyzer::analyzeDetection(
    const std::string& techniqueId,
    const std::vector<std::string>& edrAlerts)
{
    DetectionResult result;
    result.techniqueId    = techniqueId;
    result.detected       = false;
    result.confidenceScore = 0.0;

    // Set detection method from known table, fallback to "Behavioral"
    auto mit = TECHNIQUE_METHODS.find(techniqueId);
    result.detectionMethod = (mit != TECHNIQUE_METHODS.end())
        ? mit->second : "Behavioral";

    double totalWeight = 0.0;
    double matchWeight = 0.0;

    for (const auto& alert : edrAlerts) {
        std::string lower = alert;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        for (const auto& [kw, weight] : ALERT_KEYWORDS) {
            if (lower.find(kw) != std::string::npos) {
                matchWeight = std::max(matchWeight, weight);
                result.matchedPatterns.push_back(kw);
                totalWeight += weight;
            }
        }
    }

    // Remove duplicate patterns
    auto& p = result.matchedPatterns;
    std::sort(p.begin(), p.end());
    p.erase(std::unique(p.begin(), p.end()), p.end());

    if (!edrAlerts.empty()) {
        result.confidenceScore = std::min(1.0, matchWeight);
        result.detected        = (result.confidenceScore >= 0.50);
    }

    // Determine EDR response category
    if (!result.detected) {
        result.edrResponse = "Allow";
    } else if (result.confidenceScore >= 0.85) {
        result.edrResponse = "Block";
    } else {
        result.edrResponse = "Alert";
    }

    results_.push_back(result);
    return result;
}

// ----------------------------------------------------------------------------
// Analyse a complete ExploitResult (called by MLEngine::analyze)
// ----------------------------------------------------------------------------

DetectionResult DetectionAnalyzer::analyzeExploitResult(
    const edr::exploits::ExploitResult& exploit)
{
    // Combine output text, artifacts, and other alert sources
    // ExploitResult::artifacts is std::vector<std::string> (file paths,
    // registry keys, or alert strings recorded during execution).
    std::vector<std::string> allAlerts = exploit.artifacts;

    // Also treat the output string as an alert source if non-empty
    if (!exploit.output.empty()) {
        allAlerts.push_back(exploit.output);
    }
    if (!exploit.errorMessage.empty()) {
        allAlerts.push_back(exploit.errorMessage);
    }

    DetectionResult result = analyzeDetection(exploit.techniqueId, allAlerts);

    // Override: if exploit.detected is explicitly set, trust it
    if (exploit.detected && !result.detected) {
        result.detected       = true;
        result.confidenceScore = std::max(result.confidenceScore, 0.60);
        result.edrResponse    = "Alert";
    }

    return result;
}

bool DetectionAnalyzer::loadPatterns(const std::string& /*patternFile*/) {
    // Patterns are currently compiled-in; file loading reserved for JSON/YAML import
    std::cout << "[DetectionAnalyzer] Compiled-in patterns active. "
                 "File-based loading not yet implemented." << std::endl;
    return true;
}

bool DetectionAnalyzer::trainModel(
    const std::vector<std::pair<std::string, bool>>& /*trainingData*/) {
    // Statistical model training reserved for future ML model integration.
    // Currently ruled-based confidence is deterministic.
    std::cout << "[DetectionAnalyzer] Rule-based engine active. "
                 "ML training reserved for v2." << std::endl;
    return false;
}

const std::vector<DetectionResult>& DetectionAnalyzer::getResults() const {
    return results_;
}

} // namespace ml
} // namespace edr
