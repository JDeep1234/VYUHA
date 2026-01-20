/*
 * Interactive CLI - Implementation
 * ==================================
 * Author: Jdeep
 * 
 * Menu-based interactive terminal with numbered options.
 * ASCII-compatible version for MinGW.
 */

#include "cli/cli.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#define SLEEP_MS(x) Sleep(x)
#else
#include <unistd.h>
#include <termios.h>
#define SLEEP_MS(x) usleep((x) * 1000)
#endif

namespace edr {
namespace cli {

// ============================================================================
// UI IMPLEMENTATION
// ============================================================================

void UI::enableAnsi() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    SetConsoleOutputCP(CP_UTF8);
#endif
}

void UI::print(const std::string& text, const char* color) {
    std::cout << color << text << colors::RESET;
}

void UI::println(const std::string& text, const char* color) {
    std::cout << color << text << colors::RESET << std::endl;
}

void UI::info(const std::string& text) {
    std::cout << colors::CYAN << "[*] " << colors::RESET << text << std::endl;
}

void UI::success(const std::string& text) {
    std::cout << colors::BRIGHT_GREEN << "[+] " << colors::RESET << text << std::endl;
}

void UI::warning(const std::string& text) {
    std::cout << colors::BRIGHT_YELLOW << "[!] " << colors::RESET << text << std::endl;
}

void UI::error(const std::string& text) {
    std::cout << colors::BRIGHT_RED << "[-] " << colors::RESET << text << std::endl;
}

void UI::debug(const std::string& text) {
    std::cout << colors::DIM << "[.] " << text << colors::RESET << std::endl;
}

void UI::box(const std::string& title, const std::string& content) {
    int width = 60;
    std::string topBorder = "+" + std::string(width - 2, '-') + "+";
    std::string bottomBorder = "+" + std::string(width - 2, '-') + "+";
    
    std::cout << colors::CYAN << topBorder << colors::RESET << std::endl;
    
    // Title
    int padding = (width - 2 - static_cast<int>(title.length())) / 2;
    std::cout << colors::CYAN << "|" << colors::RESET;
    std::cout << std::string(padding, ' ');
    std::cout << colors::BOLD << colors::BRIGHT_CYAN << title << colors::RESET;
    std::cout << std::string(width - 2 - padding - static_cast<int>(title.length()), ' ');
    std::cout << colors::CYAN << "|" << colors::RESET << std::endl;
    
    // Divider
    std::cout << colors::CYAN << "+" << std::string(width - 2, '-') << "+" << colors::RESET << std::endl;
    
    // Content (split by newlines)
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        std::cout << colors::CYAN << "| " << colors::RESET;
        std::cout << line;
        int linePadding = width - 3 - static_cast<int>(line.length());
        if (linePadding > 0) std::cout << std::string(linePadding, ' ');
        std::cout << colors::CYAN << "|" << colors::RESET << std::endl;
    }
    
    std::cout << colors::CYAN << bottomBorder << colors::RESET << std::endl;
}

void UI::panel(const std::string& text, const char* borderColor) {
    std::cout << borderColor << "| " << colors::RESET << text << std::endl;
}

void UI::divider(const char* color) {
    std::cout << color << "------------------------------------------------------------" 
              << colors::RESET << std::endl;
}

void UI::spinner(const std::string& text) {
    const char* frames[] = {"-", "\\", "|", "/"};
    static int frame = 0;
    std::cout << "\r" << colors::CYAN << frames[frame % 4] << colors::RESET 
              << " " << text << std::flush;
    frame++;
}

void UI::progress(int current, int total, const std::string& label) {
    int width = 40;
    float progress = static_cast<float>(current) / total;
    int filled = static_cast<int>(progress * width);
    
    std::cout << "\r" << colors::DIM << "[" << colors::RESET;
    std::cout << colors::BRIGHT_GREEN << std::string(filled, '#');
    std::cout << colors::DIM << std::string(width - filled, '-');
    std::cout << colors::DIM << "]" << colors::RESET;
    std::cout << " " << static_cast<int>(progress * 100) << "% " << label << std::flush;
    
    if (current >= total) std::cout << std::endl;
}

void UI::table(const std::vector<std::vector<std::string>>& data,
               const std::vector<std::string>& headers) {
    if (data.empty()) return;
    
    // Calculate column widths
    std::vector<size_t> widths;
    size_t numCols = headers.empty() ? data[0].size() : headers.size();
    
    for (size_t i = 0; i < numCols; ++i) {
        size_t maxWidth = headers.empty() ? 0 : headers[i].length();
        for (const auto& row : data) {
            if (i < row.size()) {
                maxWidth = std::max(maxWidth, row[i].length());
            }
        }
        widths.push_back(maxWidth + 2);
    }
    
    // Print headers
    if (!headers.empty()) {
        std::cout << colors::BOLD;
        for (size_t i = 0; i < headers.size(); ++i) {
            std::cout << std::left << std::setw(static_cast<int>(widths[i])) << headers[i];
        }
        std::cout << colors::RESET << std::endl;
        
        // Header underline
        std::cout << colors::DIM;
        for (size_t i = 0; i < widths.size(); ++i) {
            std::cout << std::string(widths[i], '-');
        }
        std::cout << colors::RESET << std::endl;
    }
    
    // Print data rows
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); ++i) {
            std::cout << std::left << std::setw(static_cast<int>(widths[i])) << row[i];
        }
        std::cout << std::endl;
    }
}

std::string UI::prompt(const std::string& message) {
    std::cout << colors::BRIGHT_CYAN << "? " << colors::RESET << message << " ";
    std::string input;
    std::getline(std::cin, input);
    return input;
}

bool UI::confirm(const std::string& message) {
    std::cout << colors::BRIGHT_YELLOW << "? " << colors::RESET << message 
              << colors::DIM << " (y/n) " << colors::RESET;
    std::string input;
    std::getline(std::cin, input);
    return !input.empty() && (input[0] == 'y' || input[0] == 'Y');
}

void UI::clear() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void UI::banner() {
    std::cout << colors::BRIGHT_CYAN << R"(
    ___________  __________    ___________  ___    __  _________
   / ____/ __ \/ ____/  _/   / ____/ __ \/   |  /  |/  / ____/
  / __/ / / / / __/  / /    / /_  / /_/ / /| | / /|_/ / __/   
 / /___/ /_/ / /____/ /    / __/ / _, _/ ___ |/ /  / / /___   
/_____/_____/_____/___/   /_/   /_/ |_/_/  |_/_/  /_/_____/   
                                                              
)" << colors::RESET;

    std::cout << colors::DIM << "    APT Simulation & EDR Evasion Assessment Platform v2.0" 
              << colors::RESET << std::endl;
    std::cout << colors::DIM << "    Team: " << colors::RESET 
              << colors::BRIGHT_GREEN << "Jdeep" << colors::RESET << " | "
              << colors::BRIGHT_YELLOW << "Bipin" << colors::RESET << " | "
              << colors::BRIGHT_MAGENTA << "Karthik" << colors::RESET << std::endl;
    std::cout << std::endl;
}

// ============================================================================
// CLI IMPLEMENTATION
// ============================================================================

CLI::CLI() : running_(false) {
    UI::enableAnsi();
    registerBuiltinCommands();
    
    // Generate session ID
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    currentSession_ = oss.str();
}

CLI::~CLI() = default;

void CLI::registerBuiltinCommands() {
    // Commands are registered for internal use
    registerCommand({"help", "h", "Show help", "/help", [this](const CommandContext& ctx) { showHelp(ctx); }});
    registerCommand({"run", "r", "Run technique", "/run", [this](const CommandContext& ctx) { cmdRun(ctx); }});
    registerCommand({"list", "ls", "List techniques", "/list", [this](const CommandContext& ctx) { cmdList(ctx); }});
    registerCommand({"status", "st", "Show status", "/status", [this](const CommandContext& ctx) { cmdStatus(ctx); }});
    registerCommand({"campaign", "c", "Run campaign", "/campaign", [this](const CommandContext& ctx) { cmdCampaign(ctx); }});
    registerCommand({"snapshot", "snap", "Manage snapshots", "/snapshot", [this](const CommandContext& ctx) { cmdSnapshot(ctx); }});
    registerCommand({"clean", "", "Cleanup", "/clean", [this](const CommandContext& ctx) { cmdClean(ctx); }});
    registerCommand({"config", "cfg", "Configuration", "/config", [this](const CommandContext& ctx) { cmdConfig(ctx); }});
    registerCommand({"history", "", "History", "/history", [this](const CommandContext& ctx) { cmdHistory(ctx); }});
    registerCommand({"clear", "cls", "Clear screen", "/clear", [this](const CommandContext& ctx) { cmdClear(ctx); }});
    registerCommand({"exit", "q", "Exit", "/exit", [this](const CommandContext& ctx) { cmdExit(ctx); }});
    aliases_["quit"] = "exit";
}

void CLI::registerCommand(const Command& cmd) {
    commands_[cmd.name] = cmd;
    if (!cmd.alias.empty()) {
        aliases_[cmd.alias] = cmd.name;
    }
}

void CLI::showMainMenu() {
    std::cout << std::endl;
    std::cout << colors::BOLD << colors::BRIGHT_CYAN << "  ========== MAIN MENU ==========" << colors::RESET << std::endl;
    std::cout << std::endl;
    
    std::cout << colors::BRIGHT_GREEN << "  [1]" << colors::RESET << " Run Attack Technique" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [2]" << colors::RESET << " List Available Techniques" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [3]" << colors::RESET << " Run Attack Campaign" << std::endl;
    std::cout << std::endl;
    
    std::cout << colors::BRIGHT_YELLOW << "  [4]" << colors::RESET << " System & EDR Status" << std::endl;
    std::cout << colors::BRIGHT_YELLOW << "  [5]" << colors::RESET << " Manage Snapshots" << std::endl;
    std::cout << colors::BRIGHT_YELLOW << "  [6]" << colors::RESET << " Clean Artifacts" << std::endl;
    std::cout << std::endl;
    
    std::cout << colors::BRIGHT_MAGENTA << "  [7]" << colors::RESET << " View Configuration" << std::endl;
    std::cout << colors::BRIGHT_MAGENTA << "  [8]" << colors::RESET << " Command History" << std::endl;
    std::cout << colors::BRIGHT_MAGENTA << "  [9]" << colors::RESET << " Clear Screen" << std::endl;
    std::cout << std::endl;
    
    std::cout << colors::BRIGHT_RED << "  [0]" << colors::RESET << " Exit" << std::endl;
    std::cout << std::endl;
    
    std::cout << colors::BOLD << colors::BRIGHT_CYAN << "  ================================" << colors::RESET << std::endl;
    std::cout << std::endl;
}

void CLI::showTechniqueMenu() {
    std::cout << std::endl;
    std::cout << colors::BOLD << colors::BRIGHT_CYAN << "  ===== SELECT TECHNIQUE =====" << colors::RESET << std::endl;
    std::cout << std::endl;
    
    std::cout << colors::BRIGHT_GREEN << "  [1]" << colors::RESET << " T1055     - Process Injection" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [2]" << colors::RESET << " T1055.012 - Process Hollowing" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [3]" << colors::RESET << " T1218.002 - Control Panel (CPL)" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [4]" << colors::RESET << " T1218.005 - Mshta (HTA)" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [5]" << colors::RESET << " T1574.002 - DLL Side-Loading" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [6]" << colors::RESET << " T1106     - Direct Syscalls" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [7]" << colors::RESET << " T1562.001 - EDR Redirection" << std::endl;
    std::cout << std::endl;
    std::cout << colors::BRIGHT_YELLOW << "  [8]" << colors::RESET << " Run ALL techniques" << std::endl;
    std::cout << colors::BRIGHT_RED << "  [0]" << colors::RESET << " Back to Main Menu" << std::endl;
    std::cout << std::endl;
}

void CLI::showSnapshotMenu() {
    std::cout << std::endl;
    std::cout << colors::BOLD << colors::BRIGHT_CYAN << "  ===== SNAPSHOT MANAGER =====" << colors::RESET << std::endl;
    std::cout << std::endl;
    
    std::cout << colors::BRIGHT_GREEN << "  [1]" << colors::RESET << " Create Snapshot" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [2]" << colors::RESET << " Restore Snapshot" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [3]" << colors::RESET << " List Snapshots" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [4]" << colors::RESET << " Delete Snapshot" << std::endl;
    std::cout << std::endl;
    std::cout << colors::BRIGHT_RED << "  [0]" << colors::RESET << " Back to Main Menu" << std::endl;
    std::cout << std::endl;
}

void CLI::showWelcome() {
    UI::banner();
    UI::divider(colors::CYAN);
    std::cout << std::endl;
    
    std::cout << colors::BRIGHT_GREEN << "  Welcome to EDR Framework!" << colors::RESET << std::endl;
    std::cout << colors::DIM << "  Select an option by entering its number" << colors::RESET << std::endl;
    std::cout << std::endl;
}

void CLI::showHelp(const CommandContext& ctx) {
    showMainMenu();
}

void CLI::printPrompt() {
    std::cout << colors::BRIGHT_GREEN << "edr" << colors::RESET 
              << colors::DIM << "@" << colors::RESET
              << colors::BRIGHT_CYAN << "framework" << colors::RESET
              << colors::BRIGHT_YELLOW << " > " << colors::RESET;
}

std::string CLI::readLine() {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

CommandContext CLI::parseInput(const std::string& input) {
    CommandContext ctx;
    std::istringstream stream(input);
    std::string token;
    
    while (stream >> token) {
        ctx.args.push_back(token);
    }
    
    return ctx;
}

bool CLI::processInput(const std::string& input) {
    std::string trimmed = input;
    size_t start = trimmed.find_first_not_of(" \t");
    size_t end = trimmed.find_last_not_of(" \t");
    if (start == std::string::npos) return true;
    trimmed = trimmed.substr(start, end - start + 1);
    
    if (trimmed.empty()) return true;
    
    // Add to history
    history_.push_back(trimmed);
    
    // Check for number input
    if (trimmed.length() == 1 && trimmed[0] >= '0' && trimmed[0] <= '9') {
        int choice = trimmed[0] - '0';
        handleMainMenuChoice(choice);
        return true;
    }
    
    // Check if it's a slash command (legacy support)
    if (trimmed[0] == '/') {
        std::string cmdLine = trimmed.substr(1);
        CommandContext ctx = parseInput(cmdLine);
        
        if (ctx.args.empty()) return true;
        
        std::string cmdName = ctx.args[0];
        ctx.args.erase(ctx.args.begin());
        
        if (aliases_.count(cmdName)) {
            cmdName = aliases_[cmdName];
        }
        
        if (commands_.count(cmdName)) {
            commands_[cmdName].handler(ctx);
        } else {
            UI::error("Unknown command: /" + cmdName);
        }
    } else {
        UI::warning("Invalid input. Enter a number (0-9) or use /help");
    }
    
    return true;
}

void CLI::handleMainMenuChoice(int choice) {
    CommandContext ctx;
    
    switch (choice) {
        case 1: // Run Attack Technique
            handleTechniqueMenu();
            break;
            
        case 2: // List Techniques
            cmdList(ctx);
            break;
            
        case 3: // Run Campaign
            handleCampaignMenu();
            break;
            
        case 4: // Status
            cmdStatus(ctx);
            break;
            
        case 5: // Snapshots
            handleSnapshotMenu();
            break;
            
        case 6: // Clean
            cmdClean(ctx);
            break;
            
        case 7: // Config
            cmdConfig(ctx);
            break;
            
        case 8: // History
            cmdHistory(ctx);
            break;
            
        case 9: // Clear
            cmdClear(ctx);
            break;
            
        case 0: // Exit
            cmdExit(ctx);
            break;
            
        default:
            UI::error("Invalid choice");
            break;
    }
}

void CLI::handleTechniqueMenu() {
    showTechniqueMenu();
    
    std::cout << colors::BRIGHT_YELLOW << "  Enter choice: " << colors::RESET;
    std::string input = readLine();
    
    if (input.empty() || input[0] == '0') return;
    
    std::string techniqueId;
    int choice = std::atoi(input.c_str());
    
    switch (choice) {
        case 1: techniqueId = "T1055"; break;
        case 2: techniqueId = "T1055.012"; break;
        case 3: techniqueId = "T1218.002"; break;
        case 4: techniqueId = "T1218.005"; break;
        case 5: techniqueId = "T1574.002"; break;
        case 6: techniqueId = "T1106"; break;
        case 7: techniqueId = "T1562.001"; break;
        case 8: 
            // Run all
            UI::info("Running all techniques sequentially...");
            {
                std::vector<std::string> allTechniques = {
                    "T1055", "T1055.012", "T1218.002", "T1218.005", 
                    "T1574.002", "T1106", "T1562.001"
                };
                for (const auto& t : allTechniques) {
                    CommandContext ctx;
                    ctx.args.push_back(t);
                    cmdRun(ctx);
                    SLEEP_MS(500);
                }
            }
            return;
        default:
            UI::error("Invalid technique selection");
            return;
    }
    
    CommandContext ctx;
    ctx.args.push_back(techniqueId);
    cmdRun(ctx);
}

void CLI::handleCampaignMenu() {
    std::cout << std::endl;
    std::cout << colors::BOLD << colors::BRIGHT_CYAN << "  ===== ATTACK CAMPAIGNS =====" << colors::RESET << std::endl;
    std::cout << std::endl;
    
    std::cout << colors::BRIGHT_GREEN << "  [1]" << colors::RESET << " APT29 (Cozy Bear) Campaign" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [2]" << colors::RESET << " APT28 (Fancy Bear) Campaign" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [3]" << colors::RESET << " FIN7 Campaign" << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [4]" << colors::RESET << " Custom Campaign (from file)" << std::endl;
    std::cout << std::endl;
    std::cout << colors::BRIGHT_RED << "  [0]" << colors::RESET << " Back to Main Menu" << std::endl;
    std::cout << std::endl;
    
    std::cout << colors::BRIGHT_YELLOW << "  Enter choice: " << colors::RESET;
    std::string input = readLine();
    
    if (input.empty() || input[0] == '0') return;
    
    int choice = std::atoi(input.c_str());
    std::string campaignName;
    
    switch (choice) {
        case 1: campaignName = "apt29_cozy_bear.yaml"; break;
        case 2: campaignName = "apt28_fancy_bear.yaml"; break;
        case 3: campaignName = "fin7.yaml"; break;
        case 4:
            campaignName = UI::prompt("Enter campaign file path:");
            break;
        default:
            UI::error("Invalid campaign selection");
            return;
    }
    
    CommandContext ctx;
    ctx.args.push_back(campaignName);
    cmdCampaign(ctx);
}

void CLI::handleSnapshotMenu() {
    showSnapshotMenu();
    
    std::cout << colors::BRIGHT_YELLOW << "  Enter choice: " << colors::RESET;
    std::string input = readLine();
    
    if (input.empty() || input[0] == '0') return;
    
    int choice = std::atoi(input.c_str());
    CommandContext ctx;
    
    switch (choice) {
        case 1: {
            std::string name = UI::prompt("Enter snapshot name:");
            ctx.args.push_back("create");
            ctx.args.push_back(name.empty() ? "snapshot_" + currentSession_ : name);
            cmdSnapshot(ctx);
            break;
        }
        case 2: {
            std::string name = UI::prompt("Enter snapshot name to restore:");
            ctx.args.push_back("restore");
            ctx.args.push_back(name);
            cmdSnapshot(ctx);
            break;
        }
        case 3:
            ctx.args.push_back("list");
            cmdSnapshot(ctx);
            break;
        case 4: {
            std::string name = UI::prompt("Enter snapshot name to delete:");
            UI::warning("Delete snapshot: " + name + " (placeholder)");
            break;
        }
        default:
            UI::error("Invalid choice");
            break;
    }
}

void CLI::runInteractive() {
    running_ = true;
    showWelcome();
    showMainMenu();
    
    while (running_) {
        printPrompt();
        std::string input = readLine();
        
        if (std::cin.eof()) {
            running_ = false;
            break;
        }
        
        processInput(input);
        
        // Show menu again after each action
        if (running_) {
            showMainMenu();
        }
    }
    
    std::cout << std::endl;
    UI::success("Goodbye!");
}

int CLI::runCommand(int argc, char* argv[]) {
    if (argc < 2) {
        runInteractive();
        return 0;
    }
    
    UI::enableAnsi();
    
    std::string arg = argv[1];
    if (arg == "--help" || arg == "-h") {
        UI::banner();
        showMainMenu();
        return 0;
    }
    
    if (arg == "--version" || arg == "-v") {
        std::cout << "EDR Framework v2.0.0" << std::endl;
        return 0;
    }
    
    if (arg == "-i" || arg == "--interactive") {
        runInteractive();
        return 0;
    }
    
    // Build command string from args
    std::string cmdLine = "/" + arg;
    for (int i = 2; i < argc; ++i) {
        cmdLine += " ";
        cmdLine += argv[i];
    }
    
    UI::banner();
    processInput(cmdLine);
    
    return 0;
}

// ============================================================================
// COMMAND HANDLERS
// ============================================================================

void CLI::cmdRun(const CommandContext& ctx) {
    if (ctx.args.empty()) {
        handleTechniqueMenu();
        return;
    }
    
    std::string techniqueId = ctx.args[0];
    
    std::cout << std::endl;
    UI::box("Executing Technique", techniqueId);
    std::cout << std::endl;
    
    // Simulate execution with progress
    UI::info("Preparing environment...");
    SLEEP_MS(500);
    
    UI::info("Creating snapshot...");
    SLEEP_MS(300);
    
    UI::info("Executing technique " + techniqueId + "...");
    for (int i = 0; i <= 100; i += 10) {
        UI::progress(i, 100, techniqueId);
        SLEEP_MS(100);
    }
    
    std::cout << std::endl;
    UI::warning("Awaiting Bipin's implementation for actual exploit execution");
    std::cout << std::endl;
    
    // Show result table
    std::vector<std::string> headers = {"Field", "Value"};
    std::vector<std::vector<std::string>> data = {
        {"Technique", techniqueId},
        {"Status", "Placeholder"},
        {"EDR Alert", "N/A"},
        {"Duration", "1.2s"},
    };
    UI::table(data, headers);
    std::cout << std::endl;
}

void CLI::cmdList(const CommandContext& ctx) {
    std::cout << std::endl;
    
    std::vector<std::string> headers = {"#", "ID", "Name", "Tactic", "Owner"};
    std::vector<std::vector<std::string>> data = {
        {"1", "T1055", "Process Injection", "Defense Evasion", "Bipin"},
        {"2", "T1055.012", "Process Hollowing", "Defense Evasion", "Bipin"},
        {"3", "T1218.002", "Control Panel", "Defense Evasion", "Bipin"},
        {"4", "T1218.005", "Mshta", "Defense Evasion", "Bipin"},
        {"5", "T1574.002", "DLL Side-Loading", "Persistence", "Bipin"},
        {"6", "T1106", "Direct Syscalls", "Execution", "Bipin"},
        {"7", "T1562.001", "EDR Redirection", "Defense Evasion", "Bipin"},
    };
    
    UI::info("Available Techniques (Awaiting Implementation)");
    std::cout << std::endl;
    UI::table(data, headers);
    std::cout << std::endl;
    
    UI::warning("All techniques are placeholders - awaiting Bipin's code");
    std::cout << std::endl;
}

void CLI::cmdStatus(const CommandContext& ctx) {
    std::cout << std::endl;
    UI::info("System Status");
    std::cout << std::endl;
    
    // EDR Status
    std::vector<std::string> headers = {"Component", "Status", "Details"};
    std::vector<std::vector<std::string>> data = {
        {"Windows Defender", "Active", "Real-time protection ON"},
        {"CrowdStrike", "Not Detected", "-"},
        {"Carbon Black", "Not Detected", "-"},
        {"VM Provider", "Hyper-V", "Detected"},
    };
    
    UI::table(data, headers);
    std::cout << std::endl;
    
    UI::success("Session ID: " + currentSession_);
    std::cout << std::endl;
}

void CLI::cmdCampaign(const CommandContext& ctx) {
    if (ctx.args.empty()) {
        handleCampaignMenu();
        return;
    }
    
    std::cout << std::endl;
    UI::info("Loading campaign: " + ctx.args[0]);
    UI::warning("Campaign execution awaiting Bipin's exploit implementations");
    std::cout << std::endl;
}

void CLI::cmdSnapshot(const CommandContext& ctx) {
    if (ctx.args.empty()) {
        handleSnapshotMenu();
        return;
    }
    
    std::string action = ctx.args[0];
    
    std::cout << std::endl;
    
    if (action == "create") {
        std::string name = ctx.args.size() > 1 ? ctx.args[1] : "snapshot_" + currentSession_;
        UI::info("Creating snapshot: " + name);
        SLEEP_MS(500);
        UI::success("Snapshot created");
    } else if (action == "restore") {
        if (ctx.args.size() < 2) {
            UI::error("Specify snapshot name to restore");
            return;
        }
        UI::info("Restoring snapshot: " + ctx.args[1]);
        SLEEP_MS(500);
        UI::success("Snapshot restored");
    } else {
        UI::info("Available snapshots:");
        std::cout << colors::DIM << "  (No snapshots found)" << colors::RESET << std::endl;
    }
    
    std::cout << std::endl;
}

void CLI::cmdClean(const CommandContext& ctx) {
    std::cout << std::endl;
    
    std::cout << colors::BOLD << colors::BRIGHT_CYAN << "  ===== CLEANUP OPTIONS =====" << colors::RESET << std::endl;
    std::cout << std::endl;
    std::cout << colors::BRIGHT_GREEN << "  [1]" << colors::RESET << " Clean session artifacts only" << std::endl;
    std::cout << colors::BRIGHT_YELLOW << "  [2]" << colors::RESET << " Deep clean (restore from snapshot)" << std::endl;
    std::cout << colors::BRIGHT_RED << "  [0]" << colors::RESET << " Cancel" << std::endl;
    std::cout << std::endl;
    
    std::cout << colors::BRIGHT_YELLOW << "  Enter choice: " << colors::RESET;
    std::string input = readLine();
    
    if (input.empty() || input[0] == '0') return;
    
    int choice = std::atoi(input.c_str());
    
    if (choice == 1) {
        UI::info("Cleaning session artifacts...");
        SLEEP_MS(300);
        UI::success("Session cleaned");
    } else if (choice == 2) {
        if (UI::confirm("This will restore from last snapshot. Continue?")) {
            UI::info("Performing deep clean...");
            SLEEP_MS(500);
            UI::success("System restored from snapshot");
        }
    }
    
    std::cout << std::endl;
}

void CLI::cmdConfig(const CommandContext& ctx) {
    std::cout << std::endl;
    UI::info("Current Configuration");
    std::cout << std::endl;
    
    std::vector<std::string> headers = {"Key", "Value"};
    std::vector<std::vector<std::string>> data = {
        {"debug", "false"},
        {"log_level", "INFO"},
        {"output_dir", "results"},
        {"auto_cleanup", "true"},
        {"snapshot_provider", "auto"},
    };
    UI::table(data, headers);
    std::cout << std::endl;
}

void CLI::cmdHistory(const CommandContext& ctx) {
    std::cout << std::endl;
    UI::info("Command History");
    std::cout << std::endl;
    
    if (history_.empty()) {
        std::cout << colors::DIM << "  (No commands yet)" << colors::RESET << std::endl;
    } else {
        for (size_t i = 0; i < history_.size(); ++i) {
            std::cout << colors::DIM << "  " << (i + 1) << ". " 
                      << colors::RESET << history_[i] << std::endl;
        }
    }
    
    std::cout << std::endl;
}

void CLI::cmdClear(const CommandContext& ctx) {
    UI::clear();
    UI::banner();
}

void CLI::cmdExit(const CommandContext& ctx) {
    running_ = false;
}

} // namespace cli
} // namespace edr
