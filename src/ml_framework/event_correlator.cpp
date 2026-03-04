/*
 * ============================================================================
 * EVENT CORRELATOR - Implementation
 * ============================================================================
 * Author: Karthik
 *
 * Maps execution events to MITRE ATT&CK technique entries and correlates
 * related events into attack-chain narratives.
 *
 * ATT&CK database:
 *   - Static C++ table covers techniques used by Bipin's exploits
 *   - Extended via loadATTACKDatabase() from JSON at runtime
 * ============================================================================
 */

#include "ml_framework/ml_engine.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace edr {
namespace ml {

// ============================================================================
// Static ATT&CK knowledge base (covers techniques in ExploitManager)
// ============================================================================

static const std::vector<ATTACKMapping> STATIC_ATTACK_DB = {
    {
        "T1068", "Exploitation for Privilege Escalation",
        "Privilege Escalation",
        {"Exploitation", "Installation"},
        {"T1574.002", "T1055"},
        "M1048: Application Isolation and Sandboxing; M1078: Valid Accounts"
    },
    {
        "T1562.001", "Impair Defenses: Disable or Modify Tools",
        "Defense Evasion",
        {"Defense Evasion"},
        {"T1562.006", "T1562.002"},
        "M1022: Restrict File and Directory Permissions; M1024: Restrict Registry Permissions"
    },
    {
        "T1055", "Process Injection",
        "Defense Evasion, Privilege Escalation",
        {"Exploitation", "Installation"},
        {"T1055.001", "T1055.012", "T1068"},
        "M1040: Behavior Prevention on Endpoint"
    },
    {
        "T1055.001", "Process Injection: Dynamic-link Library Injection",
        "Defense Evasion",
        {"Exploitation"},
        {"T1574.002"},
        "M1040: Behavior Prevention on Endpoint"
    },
    {
        "T1055.012", "Process Injection: Process Hollowing",
        "Defense Evasion",
        {"Exploitation"},
        {"T1055.001"},
        "M1040: Behavior Prevention on Endpoint"
    },
    {
        "T1218.002", "System Binary Proxy Execution: Control Panel",
        "Defense Evasion",
        {"Execution"},
        {"T1218", "T1574.002"},
        "M1038: Execution Prevention"
    },
    {
        "T1574.002", "Hijack Execution Flow: DLL Side-Loading",
        "Persistence, Defense Evasion",
        {"Installation", "Persistence"},
        {"T1055.001", "T1218.002"},
        "M1013: Application Developer Guidance; M1051: Update Software"
    },
    {
        "T1562.006", "Impair Defenses: Indicator Blocking",
        "Defense Evasion",
        {"Defense Evasion"},
        {"T1562.001"},
        "M1018: User Account Management; M1053: Data Backup"
    },
    {
        "T1134", "Access Token Manipulation",
        "Defense Evasion, Privilege Escalation",
        {"Exploitation", "Installation"},
        {"T1055", "T1068"},
        "M1018: User Account Management; M1026: Privileged Account Management"
    },
    {
        "T1562.009", "Impair Defenses: Safe Mode Boot",
        "Defense Evasion",
        {"Defense Evasion"},
        {"T1562.001"},
        "M1018: User Account Management"
    },
};

// ============================================================================
// Helper — current UTC timestamp string
// ============================================================================

static std::string utcNow() {
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time), "%Y-%m-%d %H:%M:%S UTC");
    return oss.str();
}

// ============================================================================
// EventCorrelator
// ============================================================================

EventCorrelator::EventCorrelator() {
    // Seed the runtime DB from the static table
    for (const auto& entry : STATIC_ATTACK_DB) {
        attackDatabase_[entry.techniqueId] = entry;
    }
}

void EventCorrelator::analyze(const void* /*executionResult*/) {
    std::cout << "[EventCorrelator] Use correlateExploitResult() with ExploitResult." << std::endl;
}

std::string EventCorrelator::getReport() const {
    if (correlations_.empty()) {
        return "Event Correlation: No events recorded this session.";
    }

    std::ostringstream oss;
    oss << "MITRE ATT\&CK Correlation Report\n";
    oss << std::string(50, '=') << "\n";

    for (const auto& c : correlations_) {
        oss << "Event : " << c.eventId << " [" << c.source << "]\n";
        oss << "Time  : " << c.timestamp << "\n";
        oss << "Score : " << c.correlationScore << "\n";
        oss << "ATT&CK mappings:\n";
        for (const auto& m : c.mappedTechniques) {
            oss << "  " << m.techniqueId << " : " << m.techniqueName
                << " (" << m.tactic << ")\n";
            oss << "  Mitigation: " << m.mitigation << "\n";
        }
        oss << std::string(50, '-') << "\n";
    }
    return oss.str();
}

void EventCorrelator::reset() {
    correlations_.clear();
}

// ----------------------------------------------------------------------------
// Correlate a single ExploitResult into a CorrelationEvent
// ----------------------------------------------------------------------------

CorrelationEvent EventCorrelator::correlateExploitResult(
    const edr::exploits::ExploitResult& exploit)
{
    CorrelationEvent event;
    event.source    = "ExploitManager";
    event.timestamp = utcNow();
    event.eventId   = exploit.techniqueId + "_" + event.timestamp.substr(0, 10);

    // Primary technique mapping
    ATTACKMapping primary = mapToATTACK(exploit.techniqueId);
    event.mappedTechniques.push_back(primary);

    // Related techniques (add if in DB)
    for (const auto& relId : primary.relatedTechniques) {
        auto rMap = mapToATTACK(relId);
        if (rMap.techniqueName != "Unknown") {
            event.mappedTechniques.push_back(rMap);
        }
    }

    // Correlation score — higher when more DB knowledge available
    double knowledgeScore = (primary.techniqueName != "Unknown") ? 0.8 : 0.3;
    double relatedBonus   = std::min(0.2, event.mappedTechniques.size() * 0.05);
    event.correlationScore = std::min(1.0, knowledgeScore + relatedBonus);

    correlations_.push_back(event);
    return event;
}

// ----------------------------------------------------------------------------
// ATT&CK DB lookup
// ----------------------------------------------------------------------------

ATTACKMapping EventCorrelator::mapToATTACK(const std::string& techniqueId) {
    auto it = attackDatabase_.find(techniqueId);
    if (it != attackDatabase_.end()) {
        return it->second;   // ATTACKMapping stored directly
    }

    // Not found — return empty mapping
    ATTACKMapping unknown;
    unknown.techniqueId   = techniqueId;
    unknown.techniqueName = "Unknown";
    unknown.tactic        = "Unknown";
    return unknown;
}

// ----------------------------------------------------------------------------
// Correlate a list of raw event strings
// ----------------------------------------------------------------------------

std::vector<CorrelationEvent> EventCorrelator::correlateEvents(
    const std::vector<std::string>& events)
{
    std::vector<CorrelationEvent> results;
    for (const auto& ev : events) {
        CorrelationEvent c;
        c.source    = "raw_event";
        c.eventId   = ev.substr(0, std::min(ev.size(), size_t(20)));
        c.timestamp = utcNow();

        // Search for technique IDs mentioned in the event string
        for (const auto& [id, entry] : attackDatabase_) {
            if (ev.find(id) != std::string::npos ||
                ev.find(entry.techniqueName) != std::string::npos) {
                c.mappedTechniques.push_back(entry);
            }
        }
        c.correlationScore = c.mappedTechniques.empty() ? 0.1 : 0.7;
        results.push_back(c);
        correlations_.push_back(c);
    }
    return results;
}

// ----------------------------------------------------------------------------
// Load ATT&CK database from JSON file
// Expected format: array of {"id":..., "name":..., "tactic":..., ...}
// ----------------------------------------------------------------------------

bool EventCorrelator::loadATTACKDatabase(const std::string& dbPath) {
    std::ifstream file(dbPath);
    if (!file.is_open()) {
        std::cerr << "[EventCorrelator] Cannot open ATT&CK DB: " << dbPath << std::endl;
        return false;
    }

    // Basic manual JSON parsing for the ATT&CK STIX-lite JSON format.
    // Full STIX requires a JSON library; this handles common compact exports.
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    auto extractField = [&](const std::string& src, const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":\"";
        auto pos = src.find(search);
        if (pos == std::string::npos) return "";
        pos += search.size();
        auto end = src.find('"', pos);
        return (end != std::string::npos) ? src.substr(pos, end - pos) : "";
    };

    // Very simple: count loaded techniques
    int loaded = 0;
    size_t pos = 0;
    while ((pos = content.find("\"external_id\"", pos)) != std::string::npos) {
        auto snippet = content.substr(pos, 400);
        std::string id   = extractField(snippet, "external_id");
        std::string name = extractField(snippet, "name");
        std::string tac  = extractField(snippet, "kill_chain_phase");
        if (!id.empty() && id.substr(0, 1) == "T") {
            ATTACKMapping entry;
            entry.techniqueId   = id;
            entry.techniqueName = name.empty() ? id : name;
            entry.tactic        = tac;
            if (attackDatabase_.find(id) == attackDatabase_.end()) {
                attackDatabase_[id] = entry;
                ++loaded;
            }
        }
        pos++;
    }

    std::cout << "[EventCorrelator] Loaded " << loaded
              << " additional ATT&CK entries from " << dbPath << std::endl;
    return loaded > 0;
}

// ----------------------------------------------------------------------------
// Identify attack chain from event list
// ----------------------------------------------------------------------------

std::vector<std::string> EventCorrelator::identifyAttackChain(
    const std::vector<std::string>& events)
{
    std::vector<std::string> chain;
    std::map<std::string, int> phaseOrder = {
        {"Reconnaissance",  1},
        {"Weaponization",   2},
        {"Delivery",        3},
        {"Exploitation",    4},
        {"Installation",    5},
        {"Command & Control", 6},
        {"Persistence",     6},
        {"Defense Evasion", 7},
        {"Actions on Objectives", 8},
    };

    // Collect all phases from correlated events
    std::map<int, std::string> orderedPhases;
    for (const auto& ev : events) {
        for (const auto& [id, entry] : attackDatabase_) {
            if (ev.find(id) != std::string::npos ||
                ev.find(entry.techniqueName) != std::string::npos) {
                for (const auto& phase : entry.killChainPhases) {
                    auto it = phaseOrder.find(phase);
                    int order = (it != phaseOrder.end()) ? it->second : 9;
                    orderedPhases[order] = phase + " [" + id + ": " + entry.techniqueName + "]";
                }
            }
        }
    }

    for (const auto& [order, desc] : orderedPhases) {
        chain.push_back(desc);
    }
    return chain;
}

const std::vector<CorrelationEvent>& EventCorrelator::getCorrelations() const {
    return correlations_;
}

} // namespace ml
} // namespace edr
