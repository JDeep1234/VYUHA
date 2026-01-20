/*
 * EDR Adaptive Framework - Main Entry Point
 * ==========================================
 * 
 * Authors:
 *   - Jdeep:   CLI, Agent Core, Integration
 *   - Bipin:   Exploit Scripts Module  
 *   - Karthik: ML Framework
 * 
 * Build: mkdir build && cd build && cmake .. && cmake --build .
 * Run:   ./bin/edr_framework.exe --help
 */

#include <iostream>
#include <string>
#include <vector>

// Jdeep's modules
#include "cli/cli.hpp"
#include "agent_core/agent.hpp"
#include "integration/integration_manager.hpp"

// Bipin's modules
#include "exploits/exploit_manager.hpp"

// Karthik's modules
#include "ml_framework/ml_engine.hpp"

void printBanner() {
    std::cout << R"(
    ╔═══════════════════════════════════════════════════════════════╗
    ║     ███████╗██████╗ ██████╗     ███████╗██████╗  █████╗       ║
    ║     ██╔════╝██╔══██╗██╔══██╗    ██╔════╝██╔══██╗██╔══██╗      ║
    ║     █████╗  ██║  ██║██████╔╝    █████╗  ██████╔╝███████║      ║
    ║     ██╔══╝  ██║  ██║██╔══██╗    ██╔══╝  ██╔══██╗██╔══██║      ║
    ║     ███████╗██████╔╝██║  ██║    ██║     ██║  ██║██║  ██║      ║
    ║     ╚══════╝╚═════╝ ╚═╝  ╚═╝    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝      ║
    ║                                                               ║
    ║        EDR Adaptive Framework v2.0 - APT Simulation          ║
    ║                                                               ║
    ║  Team: Jdeep | Bipin | Karthik                               ║
    ╚═══════════════════════════════════════════════════════════════╝
    )" << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        printBanner();
        
        // Initialize CLI parser (Jdeep)
        edr::cli::CLI cli;
        auto command = cli.parse(argc, argv);
        
        if (command.action == "help" || command.action.empty()) {
            cli.printHelp();
            return 0;
        }
        
        // Initialize core components
        edr::agent::AgentCore agent;
        edr::integration::IntegrationManager integrator;
        edr::exploits::ExploitManager exploits;     // Bipin's module
        edr::ml::MLEngine mlEngine;                  // Karthik's module
        
        // Execute based on command
        if (command.action == "run") {
            // Single technique execution
            std::cout << "[*] Executing technique: " << command.techniqueId << std::endl;
            
            // Create snapshot before test (Jdeep)
            integrator.createSnapshot("pre_test");
            
            // Execute exploit (Bipin's code)
            auto result = exploits.execute(command.techniqueId, command.options);
            
            // Analyze with ML (Karthik's code)
            auto analysis = mlEngine.analyze(result);
            
            // Output results (Jdeep)
            agent.outputHandler().exportResults(result, analysis, command.outputFormat);
            
            // Cleanup (Jdeep)
            if (command.autoClean) {
                agent.cleaner().cleanup();
                integrator.restoreSnapshot("pre_test");
            }
            
        } else if (command.action == "campaign") {
            // Multi-technique campaign
            agent.runCampaign(command.campaignFile, exploits, mlEngine);
            
        } else if (command.action == "list") {
            exploits.listTechniques();
            
        } else if (command.action == "status") {
            integrator.getEDRStatus();
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
}
