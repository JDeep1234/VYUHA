/*
 * Output Handler - Implementation
 * =================================
 * Author: Jdeep
 * 
 * Handles result export to various formats.
 */

#include "agent_core/agent.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

namespace edr {
namespace agent {

OutputHandler::OutputHandler() 
    : outputDir_("results") {
    // Create output directory if it doesn't exist
    if (!fs::exists(outputDir_)) {
        fs::create_directories(outputDir_);
    }
}

std::string OutputHandler::generateFilename(const std::string& prefix, const std::string& ext) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << outputDir_ << "/" << prefix << "_";
    oss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    oss << "." << ext;
    
    return oss.str();
}

bool OutputHandler::exportResults(const ExecutionResult& result,
                                   const void* analysis,
                                   const std::string& format) {
    std::cout << "[*] Exporting results in " << format << " format..." << std::endl;
    
    if (format == "json") {
        return exportJSON(generateFilename("result", "json"), result);
    } else if (format == "html") {
        return exportHTML(generateFilename("report", "html"), {result}, analysis);
    } else if (format == "stix") {
        return exportSTIX(generateFilename("stix", "json"), {result});
    }
    
    // Default to JSON
    return exportJSON(generateFilename("result", "json"), result);
}

bool OutputHandler::exportJSON(const std::string& filepath, const ExecutionResult& result) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[!] Failed to open file: " << filepath << std::endl;
        return false;
    }
    
    file << "{\n";
    file << "  \"techniqueId\": \"" << result.techniqueId << "\",\n";
    file << "  \"techniqueName\": \"" << result.techniqueName << "\",\n";
    file << "  \"success\": " << (result.success ? "true" : "false") << ",\n";
    file << "  \"duration_ms\": " << result.duration.count() << ",\n";
    file << "  \"mitreTactic\": \"" << result.mitreTactic << "\",\n";
    file << "  \"artifacts\": {\n";
    
    bool first = true;
    for (const auto& [key, value] : result.artifacts) {
        if (!first) file << ",\n";
        file << "    \"" << key << "\": \"" << value << "\"";
        first = false;
    }
    
    file << "\n  },\n";
    file << "  \"edrAlerts\": [\n";
    
    first = true;
    for (const auto& alert : result.edrAlerts) {
        if (!first) file << ",\n";
        file << "    \"" << alert << "\"";
        first = false;
    }
    
    file << "\n  ]\n";
    file << "}\n";
    
    file.close();
    std::cout << "[+] Exported to: " << filepath << std::endl;
    return true;
}

bool OutputHandler::exportCSV(const std::string& filepath, 
                               const std::vector<ExecutionResult>& results) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    // Header
    file << "TechniqueID,TechniqueName,Success,Duration_ms,MitreTactic,ErrorMessage\n";
    
    for (const auto& result : results) {
        file << result.techniqueId << ",";
        file << result.techniqueName << ",";
        file << (result.success ? "true" : "false") << ",";
        file << result.duration.count() << ",";
        file << result.mitreTactic << ",";
        file << result.errorMessage << "\n";
    }
    
    file.close();
    std::cout << "[+] Exported to: " << filepath << std::endl;
    return true;
}

bool OutputHandler::exportHTML(const std::string& filepath,
                                const std::vector<ExecutionResult>& results,
                                const void* analysis) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    file << R"(<!DOCTYPE html>
<html>
<head>
    <title>VYUHA - Analysis Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #1a1a2e; color: #eee; }
        h1 { color: #00d4ff; }
        table { border-collapse: collapse; width: 100%; margin: 20px 0; }
        th, td { border: 1px solid #444; padding: 12px; text-align: left; }
        th { background: #16213e; color: #00d4ff; }
        tr:nth-child(even) { background: #1f1f3d; }
        .success { color: #00ff88; }
        .failed { color: #ff4444; }
        .header { background: linear-gradient(90deg, #16213e, #1a1a2e); padding: 20px; border-radius: 10px; }
    </style>
</head>
<body>
    <div class="header">
        <h1>VYUHA - Analysis Report</h1>
        <p>Team: Jdeep | Bipin | Karthik</p>
    </div>
    
    <h2>Execution Results</h2>
    <table>
        <tr>
            <th>Technique ID</th>
            <th>Name</th>
            <th>Status</th>
            <th>Duration</th>
            <th>MITRE Tactic</th>
        </tr>
)";
    
    for (const auto& result : results) {
        file << "        <tr>\n";
        file << "            <td>" << result.techniqueId << "</td>\n";
        file << "            <td>" << result.techniqueName << "</td>\n";
        file << "            <td class=\"" << (result.success ? "success" : "failed") << "\">";
        file << (result.success ? "SUCCESS" : "FAILED") << "</td>\n";
        file << "            <td>" << result.duration.count() << " ms</td>\n";
        file << "            <td>" << result.mitreTactic << "</td>\n";
        file << "        </tr>\n";
    }
    
    file << R"(    </table>
    
    <!-- Karthik's ML Analysis results would go here -->
    <h2>ML Analysis (Karthik's Module)</h2>
    <p><em>Analysis data from ML Framework will be inserted here.</em></p>
    
</body>
</html>
)";
    
    file.close();
    std::cout << "[+] Exported to: " << filepath << std::endl;
    return true;
}

bool OutputHandler::exportSTIX(const std::string& filepath,
                                const std::vector<ExecutionResult>& results) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    // STIX 2.1 Bundle
    file << R"({
    "type": "bundle",
    "id": "bundle--edr-framework-results",
    "objects": [
)";
    
    bool first = true;
    for (const auto& result : results) {
        if (!first) file << ",\n";
        file << R"(        {
            "type": "attack-pattern",
            "id": "attack-pattern--)" << result.techniqueId << R"(",
            "name": ")" << result.techniqueName << R"(",
            "external_references": [
                {
                    "source_name": "mitre-attack",
                    "external_id": ")" << result.techniqueId << R"("
                }
            ]
        })";
        first = false;
    }
    
    file << R"(
    ]
}
)";
    
    file.close();
    std::cout << "[+] Exported STIX to: " << filepath << std::endl;
    return true;
}

} // namespace agent
} // namespace edr
