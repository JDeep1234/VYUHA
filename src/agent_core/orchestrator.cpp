/*
 * Orchestrator - Implementation
 * ==============================
 * Author: Jdeep
 *
 * State machine that manages execution flow for single-technique dispatch
 * and file-based campaign execution.
 *
 * For ML-guided (DQN) campaign execution, see AgentCore::runCampaign() in
 * agent.cpp, which uses Bipin's ExploitManager and Karthik's MLEngine.
 */

#include "agent_core/agent.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace edr {
namespace agent {

// ============================================================================
// State name helper (for logging)
// ============================================================================
static const char* stateName(ExecutionState s) {
    switch (s) {
        case ExecutionState::IDLE:             return "IDLE";
        case ExecutionState::INITIALIZING:     return "INITIALIZING";
        case ExecutionState::SNAPSHOT_CREATED: return "SNAPSHOT_CREATED";
        case ExecutionState::EXECUTING:        return "EXECUTING";
        case ExecutionState::ANALYZING:        return "ANALYZING";
        case ExecutionState::CLEANING:         return "CLEANING";
        case ExecutionState::COMPLETED:        return "COMPLETED";
        case ExecutionState::FAILED:           return "FAILED";
        default:                               return "UNKNOWN";
    }
}

// ============================================================================
// Orchestrator
// ============================================================================

Orchestrator::Orchestrator()
    : state_(ExecutionState::IDLE) {
}

void Orchestrator::setState(ExecutionState newState) {
    std::cout << "[Orchestrator] State: "
              << stateName(state_) << " → " << stateName(newState) << std::endl;
    state_ = newState;
    if (stateCallback_) {
        stateCallback_(state_);
    }
}

void Orchestrator::setStateCallback(std::function<void(ExecutionState)> callback) {
    stateCallback_ = callback;
}

// ----------------------------------------------------------------------------
// executeTechnique — state-machine wrapper for a single technique execution.
//
// NOTE: This is used by AgentCore::runTechnique() for ad-hoc single runs.
//       For ML-guided multi-step campaigns use AgentCore::runCampaign().
//
// The actual exploit execution takes place via AgentCore which holds
// references to ExploitManager (Bipin) and MLEngine (Karthik).  Here we only
// manage timing and state transitions; the caller is responsible for the
// real work (see agent.cpp::runCampaign for the full pipeline).
// ----------------------------------------------------------------------------

ExecutionResult Orchestrator::executeTechnique(
    const std::string& techniqueId,
    const std::map<std::string, std::string>& options)
{
    ExecutionResult result;
    result.techniqueId = techniqueId;

    auto startTime = std::chrono::steady_clock::now();

    try {
        // ---- INITIALIZING -----------------------------------------------
        setState(ExecutionState::INITIALIZING);

        // Log options
        if (!options.empty()) {
            for (const auto& [k, v] : options) {
                std::cout << "[Orchestrator]   option " << k << "=" << v << std::endl;
            }
        }

        // ---- EXECUTING -------------------------------------------------
        setState(ExecutionState::EXECUTING);
        std::cout << "[Orchestrator] Executing technique: " << techniqueId << std::endl;

        // The real execution goes through ExploitManager (Bipin).
        // For ad-hoc runTechnique() calls that don't have an ExploitManager,
        // we record a placeholder result.  The caller should use runCampaign()
        // for integrated execution.
        result.success       = false;
        result.errorMessage  = "Use AgentCore::runCampaign() for integrated execution "
                               "with ExploitManager + MLEngine.";

        // ---- ANALYZING -------------------------------------------------
        setState(ExecutionState::ANALYZING);

        // ---- COMPLETED -------------------------------------------------
        setState(ExecutionState::COMPLETED);

    } catch (const std::exception& e) {
        setState(ExecutionState::FAILED);
        result.success      = false;
        result.errorMessage = e.what();
        std::cerr << "[Orchestrator] Exception: " << e.what() << std::endl;
    }

    auto endTime = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime);

    std::cout << "[Orchestrator] Technique " << techniqueId
              << " finished in " << result.duration.count() << " ms." << std::endl;

    return result;
}

// ----------------------------------------------------------------------------
// executeCampaign — file-based sequential dispatch WITHOUT ML guidance.
//
// To run with ML guidance (DQN action selection + reward training), use
//   AgentCore::runCampaign(campaignFile, exploits, mlEngine).
// ----------------------------------------------------------------------------

std::vector<ExecutionResult> Orchestrator::executeCampaign(
    const std::string& campaignFile)
{
    std::vector<ExecutionResult> results;

    std::cout << "[Orchestrator] Loading campaign: " << campaignFile << std::endl;

    std::ifstream fin(campaignFile);
    if (!fin.is_open()) {
        std::cerr << "[Orchestrator] ERROR: Cannot open: " << campaignFile << std::endl;
        return results;
    }

    std::vector<std::string> techniques;
    std::string line;
    while (std::getline(fin, line)) {
        // Trim
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line[0] == '#') continue;
        techniques.push_back(line);
    }
    fin.close();

    std::cout << "[Orchestrator] " << techniques.size()
              << " techniques in campaign." << std::endl;

    for (size_t i = 0; i < techniques.size(); ++i) {
        std::cout << "[Orchestrator] [" << (i + 1) << "/" << techniques.size()
                  << "] " << techniques[i] << std::endl;

        ExecutionResult res = executeTechnique(techniques[i], {});
        results.push_back(res);

        // Stop campaign on unexpected failures (optional policy)
        if (!res.success && res.errorMessage.find("Exception") != std::string::npos) {
            std::cerr << "[Orchestrator] Fatal error in campaign — stopping early."
                      << std::endl;
            break;
        }
    }

    std::cout << "[Orchestrator] Campaign finished: "
              << results.size() << " results." << std::endl;

    return results;
}

} // namespace agent
} // namespace edr
