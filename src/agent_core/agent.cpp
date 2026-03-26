/*
 * Agent Core - Main Implementation
 * ==================================
 * Author: Jdeep
 *
 * runCampaign() integrates:
 *   - Bipin's ExploitManager (edr::exploits::ExploitManager)
 *   - Karthik's MLEngine    (edr::ml::MLEngine)
 */

#include "agent_core/agent.hpp"
#include "exploits/exploit_manager.hpp"
#include "ml_framework/ml_engine.hpp"
#include "ml_framework/ml_bridge.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <random>
#include <thread>
#include <algorithm>

namespace edr {
namespace agent {

// ============================================================================
// AGENT CORE IMPLEMENTATION
// ============================================================================

AgentCore::AgentCore() {
    // Generate session ID
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);

    std::ostringstream oss;
    oss << "session_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    oss << "_" << dis(gen);
    sessionId_ = oss.str();

    sessionStart_ = now;
}

bool AgentCore::initialize() {
    std::cout << "[AgentCore] Initializing..." << std::endl;
    std::cout << "[AgentCore] Session ID: " << sessionId_ << std::endl;

    if (!telemetry_.startMonitoring()) {
        std::cerr << "[AgentCore] Warning: telemetry monitoring failed to start." << std::endl;
    }

    return true;
}

ExecutionResult AgentCore::runTechnique(const std::string& techniqueId,
                                         const std::map<std::string, std::string>& options) {
    std::cout << "[AgentCore] Running technique: " << techniqueId << std::endl;
    return orchestrator_.executeTechnique(techniqueId, options);
}

// ============================================================================
// runCampaign — Jdeep + Karthik + Bipin integration point
//
// Campaign file format (simple text, one technique per line):
//   # comment lines starting with '#' are ignored
//   T1055
//   T1068
//   T1562.001
// ============================================================================

std::vector<ExecutionResult> AgentCore::runCampaign(
    const std::string& campaignFile,
    edr::exploits::ExploitManager& exploits,
    edr::ml::MLEngine& mlEngine)
{
    std::cout << "[AgentCore] Starting campaign: " << campaignFile << std::endl;

    std::vector<ExecutionResult> results;

    // ------------------------------------------------------------------
    // 1. Parse campaign file for technique IDs
    // ------------------------------------------------------------------
    std::vector<std::string> techniques;
    std::ifstream fin(campaignFile);
    if (!fin.is_open()) {
        std::cerr << "[AgentCore] ERROR: Cannot open campaign file: "
                  << campaignFile << std::endl;
        return results;
    }

    std::string line;
    while (std::getline(fin, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line[0] == '#') continue;
        techniques.push_back(line);
    }
    fin.close();

    if (techniques.empty()) {
        std::cerr << "[AgentCore] Campaign file has no techniques." << std::endl;
        return results;
    }

    std::cout << "[AgentCore] Loaded " << techniques.size()
              << " techniques from campaign." << std::endl;

    // ------------------------------------------------------------------
    // 2. Pre-run: build initial valid action index list
    // ------------------------------------------------------------------
    // Map MITRE technique IDs to DQN action indices (see DESIGN.md action space)
// Inside std::vector<ExecutionResult> AgentCore::runCampaign(...)

    // ------------------------------------------------------------------
    // 2. Pre-run: build initial valid action index list
    // ------------------------------------------------------------------
    // Strict mapping to Python action space size of 3
    static const std::map<std::string, int> TECH_TO_ACTION = {
        {"T1068",      0},  // Maps to "BYOVD_VulnDriver" in Python
        {"T1562.001",  1},  // Maps to "EDR_Freeze_Thread" in Python
        {"T1055.001",  2}   // Maps to "Crystal_Palace_Loader" in Python
    };

    std::vector<int> validActions;
    for (const auto& t : techniques) {
        if (exploits.hasTechnique(t)) { 
            auto it = TECH_TO_ACTION.find(t);
            if (it != TECH_TO_ACTION.end()) {
                validActions.push_back(it->second);
            }
        }
    }
    
    // Safety check: if an empty validActions is sent, ML bridge might crash
    if (validActions.empty()) {
        std::cerr << "[AgentCore] ERROR: No valid mapped actions found for ML." << std::endl;
        return results;
    }
        // Action-index back to technique ID (reverse map)
    std::map<int, std::string> actionToTech;
    for (const auto& [tech, idx] : TECH_TO_ACTION) {
        actionToTech[idx] = tech;
    }

    // ------------------------------------------------------------------
    // 3. Campaign loop
    // ------------------------------------------------------------------
    int step = 0;

    // Build initial system state from environment
    edr::ml::SystemState currentState;
    {
        auto edrName = telemetry_.getEDRAlerts().empty() ? "" :
                       "Defender"; // fallback: could be auto-detected
        currentState = edr::ml::SystemState::fromContext(
            /*edrName*/            edrName,
            /*edrVersion*/         "unknown",
            /*edrRunning*/         false,
            /*driverLoaded*/       false,
            /*windowsBuild*/       22000,
            /*intLevel*/           1,     // High
            /*ppl*/                true,
            /*dse*/                true,
            /*secureBoot*/         true,
            /*virt*/               false,
            /*elam*/               true,
            /*lastActionId*/       -1,
            /*lastResultId*/       -1,
            /*consecutiveFails*/   0,
            /*secondsSinceLast*/   0.0);
    }

    for (const std::string& techniqueId : techniques) {
        ++step;
        std::cout << "\n[AgentCore] --- Step " << step << "/" << techniques.size()
                  << ": " << techniqueId << " ---" << std::endl;

        // ----------------------------------------------------------------
        // 3a. Ask ML engine for the recommended DQN action (Karthik)
        // ----------------------------------------------------------------
        edr::ml::ActionResponse actionResp = mlEngine.recommendAction(
            currentState, validActions);

        // If DQN recommends a different technique and it exists in our list,
        // prefer it; otherwise stick with the campaign order.
        std::string targetTech = techniqueId;
        if (actionResp.valid) {
            auto ait = actionToTech.find(actionResp.actionId);
            if (ait != actionToTech.end()) {
                auto found = std::find(techniques.begin(), techniques.end(),
                                       ait->second);
                if (found != techniques.end()) {
                    targetTech = ait->second;
                    std::cout << "[AgentCore] DQN recommends: "
                              << actionResp.actionName
                              << " → technique " << targetTech << std::endl;
                }
            }
        }

        // ----------------------------------------------------------------
        // 3b. Execute the technique via Bipin's ExploitManager
        // ----------------------------------------------------------------
        orchestrator_.setState(ExecutionState::EXECUTING);

        std::map<std::string, std::string> options;
        options["session_id"] = sessionId_;
        // Encode target EDR name so MLEngine can extract it from artifacts
        // (Convention: push_back("edr:<name>") in artifacts for identification)

        edr::exploits::ExploitResult exploitResult =
            exploits.execute(targetTech, options);

        std::cout << "[AgentCore] Exploit result: "
                  << (exploitResult.success ? "SUCCESS" : "FAILED")
                  << (exploitResult.detected ? " [DETECTED]" : " [UNDETECTED]")
                  << std::endl;

        // ----------------------------------------------------------------
        // 3c. Collect EDR alerts from telemetry into artifacts
        // ----------------------------------------------------------------
        auto edrAlerts = telemetry_.getEDRAlerts();
        for (const auto& alert : edrAlerts) {
            exploitResult.artifacts.push_back(alert);
        }

        // ----------------------------------------------------------------
        // 3d. Build next system state snapshot
        // ----------------------------------------------------------------
        edr::ml::SystemState nextState = edr::ml::SystemState::fromContext(
            /*edrName*/          "",
            /*edrVersion*/       "unknown",
            /*edrRunning*/       !exploitResult.success,
            /*driverLoaded*/     false,
            /*windowsBuild*/     22000,
            /*intLevel*/         1,
            /*ppl*/              true,
            /*dse*/              true,
            /*secureBoot*/       true,
            /*virt*/             false,
            /*elam*/             true,
            /*lastActionId*/     actionResp.actionId,
            /*lastResultId*/     exploitResult.success ? 0 : 2,
            /*consecutiveFails*/ (int)std::count_if(
                results.begin(), results.end(),
                [](const ExecutionResult& r){ return !r.success; }),
            /*secondsSinceLast*/ 0.5);

        // ----------------------------------------------------------------
        // 3e. Analyse result via Karthik's ML engine
        // ----------------------------------------------------------------
        orchestrator_.setState(ExecutionState::ANALYZING);

        edr::ml::AnalysisReport mlReport =
            mlEngine.analyze(exploitResult, currentState, nextState);

        std::cout << "[AgentCore] ML analysis: "
                  << (mlReport.detections.empty() ? "OK" :
                      (mlReport.detections[0].detected ? "DETECTED" : "EVADED"))
                  << "  evasion-rate="
                  << std::fixed << std::setprecision(2)
                  << (mlReport.overallEvasionRate * 100.0) << "%" << std::endl;

        if (!mlReport.recommendation.empty()) {
            std::cout << "[AgentCore]   Recommendation: "
                      << mlReport.recommendation << std::endl;
        }

        // ----------------------------------------------------------------
        // 3f. Wrap ExploitResult into agent ExecutionResult
        // ----------------------------------------------------------------
        ExecutionResult agentResult;
        agentResult.techniqueId   = exploitResult.techniqueId;
        agentResult.techniqueName = exploitResult.techniqueName;
        agentResult.success       = exploitResult.success;
        agentResult.errorMessage  = exploitResult.errorMessage;
        agentResult.duration      = exploitResult.executionTime;
        agentResult.edrAlerts     = edrAlerts;
        if (!mlReport.correlations.empty()) {
            const auto& corr = mlReport.correlations[0];
            if (!corr.mappedTechniques.empty()) {
                agentResult.mitreTactic = corr.mappedTechniques[0].tactic;
            }
        }

        results.push_back(agentResult);

        // ----------------------------------------------------------------
        // 3g. State transition and delay
        // ----------------------------------------------------------------
        orchestrator_.setState(
            exploitResult.success ? ExecutionState::COMPLETED : ExecutionState::FAILED);

        currentState = nextState;

        // Small delay to let EDR settle before the next attempt
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // ------------------------------------------------------------------
    // 4. Post-campaign: save ML models + cleanup
    // ------------------------------------------------------------------
    std::cout << "\n[AgentCore] Campaign complete. " << results.size()
              << " techniques executed." << std::endl;

    mlEngine.saveModels("models/campaign_" + sessionId_);

    orchestrator_.setState(ExecutionState::CLEANING);
    cleaner_.cleanup();
    orchestrator_.setState(ExecutionState::IDLE);

    return results;
}

} // namespace agent
} // namespace edr
