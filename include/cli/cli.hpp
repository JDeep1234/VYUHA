/*
 * CLI Module - Header
 * ====================
 * Author: Jdeep
 * 
 * Command line interface for the EDR Framework.
 * Handles argument parsing and command dispatch.
 */

#ifndef EDR_CLI_HPP
#define EDR_CLI_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace edr {
namespace cli {

/**
 * Parsed command structure
 */
struct Command {
    std::string action;              // run, campaign, list, status, clean
    std::string techniqueId;         // MITRE technique ID (e.g., T1055)
    std::string campaignFile;        // Campaign YAML/JSON file
    std::string outputFormat;        // json, csv, html, stix
    std::string snapshotName;        // VM snapshot name
    bool autoClean = true;           // Auto cleanup after execution
    bool verbose = false;            // Verbose output
    bool dryRun = false;             // Dry run mode
    std::map<std::string, std::string> options;  // Additional options
};

/**
 * CLI Parser and Handler
 */
class CLI {
public:
    CLI();
    ~CLI() = default;
    
    /**
     * Parse command line arguments
     */
    Command parse(int argc, char* argv[]);
    
    /**
     * Print help message
     */
    void printHelp() const;
    
    /**
     * Print version info
     */
    void printVersion() const;
    
    /**
     * Validate command
     */
    bool validateCommand(const Command& cmd) const;

private:
    std::string programName_;
    std::string version_;
    
    void parseOptions(Command& cmd, const std::vector<std::string>& args);
};

} // namespace cli
} // namespace edr

#endif // EDR_CLI_HPP
