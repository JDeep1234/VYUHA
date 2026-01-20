/*
 * CLI Module - Implementation
 * ============================
 * Author: Jdeep
 * 
 * Command line interface implementation.
 */

#include "cli/cli.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace edr {
namespace cli {

CLI::CLI() 
    : programName_("edr_framework")
    , version_("2.0.0") {
}

Command CLI::parse(int argc, char* argv[]) {
    Command cmd;
    std::vector<std::string> args;
    
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    
    if (args.empty()) {
        cmd.action = "help";
        return cmd;
    }
    
    // Parse first argument as action
    std::string firstArg = args[0];
    
    if (firstArg == "-h" || firstArg == "--help") {
        cmd.action = "help";
        return cmd;
    }
    
    if (firstArg == "-v" || firstArg == "--version") {
        printVersion();
        cmd.action = "";
        return cmd;
    }
    
    // Valid actions
    std::vector<std::string> validActions = {
        "run", "campaign", "list", "status", "snapshot", "clean", "report"
    };
    
    if (std::find(validActions.begin(), validActions.end(), firstArg) != validActions.end()) {
        cmd.action = firstArg;
        args.erase(args.begin());
    } else {
        std::cerr << "[!] Unknown action: " << firstArg << std::endl;
        cmd.action = "help";
        return cmd;
    }
    
    // Parse remaining options
    parseOptions(cmd, args);
    
    return cmd;
}

void CLI::parseOptions(Command& cmd, const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        
        if (arg == "-t" || arg == "--technique") {
            if (i + 1 < args.size()) {
                cmd.techniqueId = args[++i];
            }
        }
        else if (arg == "-c" || arg == "--campaign") {
            if (i + 1 < args.size()) {
                cmd.campaignFile = args[++i];
            }
        }
        else if (arg == "-o" || arg == "--output") {
            if (i + 1 < args.size()) {
                cmd.outputFormat = args[++i];
            }
        }
        else if (arg == "-s" || arg == "--snapshot") {
            if (i + 1 < args.size()) {
                cmd.snapshotName = args[++i];
            }
        }
        else if (arg == "--no-clean") {
            cmd.autoClean = false;
        }
        else if (arg == "-v" || arg == "--verbose") {
            cmd.verbose = true;
        }
        else if (arg == "--dry-run") {
            cmd.dryRun = true;
        }
        else if (arg.find("--") == 0) {
            // Custom option --key=value
            size_t eqPos = arg.find('=');
            if (eqPos != std::string::npos) {
                std::string key = arg.substr(2, eqPos - 2);
                std::string value = arg.substr(eqPos + 1);
                cmd.options[key] = value;
            }
        }
    }
}

void CLI::printHelp() const {
    std::cout << R"(
EDR Adaptive Framework v)" << version_ << R"(

USAGE:
    edr_framework <ACTION> [OPTIONS]

ACTIONS:
    run         Execute a single technique
    campaign    Run a campaign file with multiple techniques
    list        List available techniques
    status      Show EDR detection status
    snapshot    Manage VM snapshots
    clean       Cleanup artifacts from previous runs
    report      Generate analysis report

OPTIONS:
    -t, --technique <ID>    MITRE technique ID (e.g., T1055, T1574.002)
    -c, --campaign <FILE>   Campaign YAML/JSON file
    -o, --output <FORMAT>   Output format: json, csv, html, stix
    -s, --snapshot <NAME>   Snapshot name for VM operations
    --no-clean              Skip automatic cleanup after execution
    -v, --verbose           Enable verbose output
    --dry-run               Simulate execution without running exploits
    -h, --help              Show this help message
    --version               Show version information

EXAMPLES:
    edr_framework run -t T1055 --verbose
    edr_framework campaign -c attacks.yaml -o html
    edr_framework list --tactic defense-evasion
    edr_framework snapshot -s clean_state
    edr_framework status
    edr_framework clean --all

TEAM:
    Jdeep   - CLI, Agent Core, Integration
    Bipin   - Exploit Scripts Module
    Karthik - ML Framework

For more information, visit: https://github.com/JDeep1234/EDR-Adaptive-Framework
)" << std::endl;
}

void CLI::printVersion() const {
    std::cout << "EDR Adaptive Framework v" << version_ << std::endl;
    std::cout << "Build: C++ 17" << std::endl;
    std::cout << "Authors: Jdeep, Bipin, Karthik" << std::endl;
}

bool CLI::validateCommand(const Command& cmd) const {
    if (cmd.action == "run" && cmd.techniqueId.empty()) {
        std::cerr << "[!] Error: 'run' requires a technique ID (-t)" << std::endl;
        return false;
    }
    
    if (cmd.action == "campaign" && cmd.campaignFile.empty()) {
        std::cerr << "[!] Error: 'campaign' requires a campaign file (-c)" << std::endl;
        return false;
    }
    
    return true;
}

} // namespace cli
} // namespace edr
