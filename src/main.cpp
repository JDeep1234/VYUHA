/*
 * VYUHA: Cross-Layer EDR Kill-Chain Evasion via Deep Reinforcement Learning-Guided Adversarial Orchestration - Main Entry Point
 * ==========================================
 * 
 * Authors:
 *   - Jdeep:   CLI, Agent Core, Integration
 *   - Bipin:   Exploit Scripts Module  
 *   - Karthik: ML Framework
 * 
 * Build: mkdir build && cd build && cmake .. && cmake --build .
 * Run:   ./bin/edr_framework.exe (interactive mode)
 *        ./bin/edr_framework.exe run T1055 (single command)
 */

#include <iostream>

// Jdeep's Interactive CLI
#include "cli/cli.hpp"

/**
 * Main entry point
 * 
 * No arguments: Launch interactive REPL mode (like Claude CLI)
 * With arguments: Execute single command
 */
int main(int argc, char* argv[]) {
    try {
        // Create CLI instance and run
        edr::cli::CLI cli;
        return cli.runCommand(argc, argv);
        
    } catch (const std::exception& e) {
        std::cerr << "\033[31m[ERROR]\033[0m " << e.what() << std::endl;
        return 1;
    }
}
