/*
 * Agent Core - Header
 * ====================
 * Author: Jdeep
 * 
 * Central agent coordinating test execution, output, and cleanup.
 */

#ifndef EDR_AGENT_HPP
#define EDR_AGENT_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <functional>

// Forward declarations for Bipin and Karthik's modules
namespace edr { 
    namespace exploits { class ExploitManager; }
    namespace ml { class MLEngine; }
}

namespace edr {
namespace agent {

// ============================================================================
// EXECUTION CONTEXT
// ============================================================================

enum class ExecutionState { 
    IDLE,
    INITIALIZING,
    SNAPSHOT_CREATED,
    EXECUTING,
    ANALYZING,
    CLEANING,
    COMPLETED,
    FAILED
};

struct ExecutionResult {
    std::string techniqueId;
    std::string techniqueName;
    bool success;
    std::string errorMessage;
    std::chrono::milliseconds duration;
    std::map<std::string, std::string> artifacts;
    std::vector<std::string> edrAlerts;
    std::string mitreTactic;
};

// ============================================================================
// ORCHESTRATOR - Execution Flow Manager
// ============================================================================

class Orchestrator {
public:
    Orchestrator();
    ~Orchestrator() = default;
    
    /**
     * Execute a single technique
     */
    ExecutionResult executeTechnique(const std::string& techniqueId,
                                     const std::map<std::string, std::string>& options);
    
    /**
     * Execute campaign from file
     */
    std::vector<ExecutionResult> executeCampaign(const std::string& campaignFile);
    
    /**
     * Get current state
     */
    ExecutionState getState() const { return state_; }
    
    /**
     * Set state change callback
     */
    void setStateCallback(std::function<void(ExecutionState)> callback);

private:
    ExecutionState state_;
    std::function<void(ExecutionState)> stateCallback_;
    
    void setState(ExecutionState newState);
};

// ============================================================================
// OUTPUT HANDLER - Results Export
// ============================================================================

struct AnalysisResult; // Forward declaration for Karthik's module

class OutputHandler {
public:
    OutputHandler();
    ~OutputHandler() = default;
    
    /**
     * Export results to file
     */
    bool exportResults(const ExecutionResult& result,
                       const void* analysis,  // Karthik's AnalysisResult
                       const std::string& format);
    
    /**
     * Export to JSON
     */
    bool exportJSON(const std::string& filepath, const ExecutionResult& result);
    
    /**
     * Export to CSV
     */
    bool exportCSV(const std::string& filepath, const std::vector<ExecutionResult>& results);
    
    /**
     * Export to HTML report
     */
    bool exportHTML(const std::string& filepath, 
                    const std::vector<ExecutionResult>& results,
                    const void* analysis);
    
    /**
     * Export to STIX 2.1 format
     */
    bool exportSTIX(const std::string& filepath, const std::vector<ExecutionResult>& results);
    
    /**
     * Set output directory
     */
    void setOutputDir(const std::string& dir) { outputDir_ = dir; }

private:
    std::string outputDir_;
    
    std::string generateFilename(const std::string& prefix, const std::string& ext);
};

// ============================================================================
// CLEANER - Artifact Removal
// ============================================================================

class Cleaner {
public:
    Cleaner();
    ~Cleaner() = default;
    
    /**
     * Cleanup all artifacts from session
     */
    bool cleanup();
    
    /**
     * Cleanup specific technique artifacts
     */
    bool cleanupTechnique(const std::string& techniqueId);
    
    /**
     * Deep clean - remove all traces
     */
    bool deepClean();
    
    /**
     * Register artifact for cleanup
     */
    void registerArtifact(const std::string& type, const std::string& path);
    
    /**
     * Clear artifact registry
     */
    void clearRegistry();

private:
    std::map<std::string, std::vector<std::string>> artifacts_;
    
    bool cleanFiles(const std::vector<std::string>& files);
    bool cleanRegistry(const std::vector<std::string>& keys);
    bool cleanServices(const std::vector<std::string>& services);
    bool cleanScheduledTasks(const std::vector<std::string>& tasks);
    bool cleanProcesses(const std::vector<std::string>& processes);
};

// ============================================================================
// TELEMETRY MONITOR - EDR Response Tracking
// ============================================================================

struct TelemetryEvent {
    std::string source;
    std::string eventType;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    int eventId;
};

class TelemetryMonitor {
public:
    TelemetryMonitor();
    ~TelemetryMonitor();
    
    /**
     * Start monitoring
     */
    bool startMonitoring();
    
    /**
     * Stop monitoring
     */
    void stopMonitoring();
    
    /**
     * Get captured events
     */
    std::vector<TelemetryEvent> getEvents() const;
    
    /**
     * Get EDR alerts
     */
    std::vector<std::string> getEDRAlerts() const;
    
    /**
     * Check if specific EDR is detected
     */
    bool isEDRDetected(const std::string& edrName) const;

private:
    bool monitoring_;
    std::vector<TelemetryEvent> events_;
    
    void monitorSecurityLog();
    void monitorEDRProcesses();
};

// ============================================================================
// AGENT CORE - Main Coordinator
// ============================================================================

class AgentCore {
public:
    AgentCore();
    ~AgentCore() = default;
    
    /**
     * Initialize agent
     */
    bool initialize();
    
    /**
     * Run single technique
     */
    ExecutionResult runTechnique(const std::string& techniqueId,
                                 const std::map<std::string, std::string>& options);
    
    /**
     * Run campaign with external modules
     * Uses Bipin's ExploitManager and Karthik's MLEngine
     */
    std::vector<ExecutionResult> runCampaign(
        const std::string& campaignFile,
        edr::exploits::ExploitManager& exploits,
        edr::ml::MLEngine& mlEngine);
    
    /**
     * Get component references
     */
    Orchestrator& orchestrator() { return orchestrator_; }
    OutputHandler& outputHandler() { return outputHandler_; }
    Cleaner& cleaner() { return cleaner_; }
    TelemetryMonitor& telemetry() { return telemetry_; }

private:
    Orchestrator orchestrator_;
    OutputHandler outputHandler_;
    Cleaner cleaner_;
    TelemetryMonitor telemetry_;
    
    std::string sessionId_;
    std::chrono::system_clock::time_point sessionStart_;
};

} // namespace agent
} // namespace edr

#endif // EDR_AGENT_HPP
