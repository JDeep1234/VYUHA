/*
 * Interactive CLI - Implementation
 * ==================================
 * Author: Jdeep
 * 
 * Claude-style interactive terminal with colors and slash commands.
 */

#include "cli/cli.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <iomanip>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
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
    
    // Set console to UTF-8
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
    std::cout << colors::CYAN << "● " << colors::RESET << text << std::endl;
}

void UI::success(const std::string& text) {
    std::cout << colors::BRIGHT_GREEN << "✓ " << colors::RESET << text << std::endl;
}

void UI::warning(const std::string& text) {
    std::cout << colors::BRIGHT_YELLOW << "⚠ " << colors::RESET << text << std::endl;
}

void UI::error(const std::string& text) {
    std::cout << colors::BRIGHT_RED << "✗ " << colors::RESET << text << std::endl;
}

void UI::debug(const std::string& text) {
    std::cout << colors::DIM << "◦ " << text << colors::RESET << std::endl;
}

void UI::box(const std::string& title, const std::string& content) {
    int width = 60;
    std::string topBorder = "╭" + std::string(width - 2, '─') + "╮";
    std::string bottomBorder = "╰" + std::string(width - 2, '─') + "╯";
    
    std::cout << colors::CYAN << topBorder << colors::RESET << std::endl;
    
    // Title
    int padding = (width - 2 - title.length()) / 2;
    std::cout << colors::CYAN << "│" << colors::RESET;
    std::cout << std::string(padding, ' ');
    std::cout << colors::BOLD << colors::BRIGHT_CYAN << title << colors::RESET;
    std::cout << std::string(width - 2 - padding - title.length(), ' ');
    std::cout << colors::CYAN << "│" << colors::RESET << std::endl;
    
    // Divider
    std::cout << colors::CYAN << "├" << std::string(width - 2, '─') << "┤" << colors::RESET << std::endl;
    
    // Content (split by newlines)
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        std::cout << colors::CYAN << "│ " << colors::RESET;
        std::cout << line;
        int linePadding = width - 3 - line.length();
        if (linePadding > 0) std::cout << std::string(linePadding, ' ');
        std::cout << colors::CYAN << "│" << colors::RESET << std::endl;
    }
    
    std::cout << colors::CYAN << bottomBorder << colors::RESET << std::endl;
}

void UI::panel(const std::string& text, const char* borderColor) {
    std::cout << borderColor << "┃ " << colors::RESET << text << std::endl;
}

void UI::divider(const char* color) {
    std::cout << color << "────────────────────────────────────────────────────────────" 
              << colors::RESET << std::endl;
}

void UI::spinner(const std::string& text) {
    const char* frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    static int frame = 0;
    std::cout << "\r" << colors::CYAN << frames[frame % 10] << colors::RESET 
              << " " << text << std::flush;
    frame++;
}

void UI::progress(int current, int total, const std::string& label) {
    int width = 40;
    float progress = static_cast<float>(current) / total;
    int filled = static_cast<int>(progress * width);
    
    std::cout << "\r" << colors::DIM << "[" << colors::RESET;
    std::cout << colors::BRIGHT_GREEN << std::string(filled, '█');
    std::cout << colors::DIM << std::string(width - filled, '░');
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
            std::cout << std::left << std::setw(widths[i]) << headers[i];
        }
        std::cout << colors::RESET << std::endl;
        
        // Header underline
        std::cout << colors::DIM;
        for (size_t w : widths) {
            std::cout << std::string(w, '─');
        }
        std::cout << colors::RESET << std::endl;
    }
    
    // Print data rows
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); ++i) {
            std::cout << std::left << std::setw(widths[i]) << row[i];
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
    ███████╗██████╗ ██████╗     ███████╗██████╗  █████╗ ███╗   ███╗███████╗
    ██╔════╝██╔══██╗██╔══██╗    ██╔════╝██╔══██╗██╔══██╗████╗ ████║██╔════╝
    █████╗  ██║  ██║██████╔╝    █████╗  ██████╔╝███████║██╔████╔██║█████╗  
    ██╔══╝  ██║  ██║██╔══██╗    ██╔══╝  ██╔══██╗██╔══██║██║╚██╔╝██║██╔══╝  
    ███████╗██████╔╝██║  ██║    ██║     ██║  ██║██║  ██║██║ ╚═╝ ██║███████╗
    ╚══════╝╚═════╝ ╚═╝  ╚═╝    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝
)" << colors::RESET << std::endl;

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
    // /help or /h
    registerCommand({
        "help", "h", 
        "Show available commands",
        "/help [command]",
        [this](const CommandContext& ctx) { showHelp(ctx); }
    });
    
    // /run or /r
    registerCommand({
        "run", "r",
        "Execute a MITRE ATT&CK technique",
        "/run <technique_id> [--target <process>] [--verbose]",
        [this](const CommandContext& ctx) { cmdRun(ctx); }
    });
    
    // /list or /ls
    registerCommand({
        "list", "ls",
        "List available techniques",
        "/list [--tactic <tactic>]",
        [this](const CommandContext& ctx) { cmdList(ctx); }
    });
    
    // /status or /st
    registerCommand({
        "status", "st",
        "Show EDR and system status",
        "/status",
        [this](const CommandContext& ctx) { cmdStatus(ctx); }
    });
    
    // /campaign or /c
    registerCommand({
        "campaign", "c",
        "Run attack campaign from file",
        "/campaign <file.yaml>",
        [this](const CommandContext& ctx) { cmdCampaign(ctx); }
    });
    
    // /snapshot or /snap
    registerCommand({
        "snapshot", "snap",
        "Manage VM snapshots",
        "/snapshot [create|restore|list] [name]",
        [this](const CommandContext& ctx) { cmdSnapshot(ctx); }
    });
    
    // /clean
    registerCommand({
        "clean", "",
        "Cleanup artifacts",
        "/clean [--all]",
        [this](const CommandContext& ctx) { cmdClean(ctx); }
    });
    
    // /config or /cfg
    registerCommand({
        "config", "cfg",
        "View or modify configuration",
        "/config [key] [value]",
        [this](const CommandContext& ctx) { cmdConfig(ctx); }
    });
    
    // /history
    registerCommand({
        "history", "",
        "Show command history",
        "/history",
        [this](const CommandContext& ctx) { cmdHistory(ctx); }
    });
    
    // /clear or /cls
    registerCommand({
        "clear", "cls",
        "Clear the screen",
        "/clear",
        [this](const CommandContext& ctx) { cmdClear(ctx); }
    });
    
    // /exit or /quit or /q
    registerCommand({
        "exit", "q",
        "Exit the framework",
        "/exit",
        [this](const CommandContext& ctx) { cmdExit(ctx); }
    });
    aliases_["quit"] = "exit";
}

void CLI::registerCommand(const Command& cmd) {
    commands_[cmd.name] = cmd;
    if (!cmd.alias.empty()) {
        aliases_[cmd.alias] = cmd.name;
    }
}

void CLI::showWelcome() {
    UI::banner();
    UI::divider(colors::CYAN);
    std::cout << std::endl;
    
    std::cout << colors::BRIGHT_GREEN << "  Welcome to EDR Framework!" << colors::RESET << std::endl;
    std::cout << colors::DIM << "  Type " << colors::RESET 
              << colors::CYAN << "/help" << colors::RESET 
              << colors::DIM << " to see available commands" << colors::RESET << std::endl;
    std::cout << colors::DIM << "  Type " << colors::RESET
              << colors::CYAN << "/exit" << colors::RESET
              << colors::DIM << " to quit" << colors::RESET << std::endl;
    std::cout << std::endl;
    UI::divider(colors::DIM);
    std::cout << std::endl;
}

void CLI::showHelp(const CommandContext& ctx) {
    if (!ctx.args.empty()) {
        // Show help for specific command
        std::string cmdName = ctx.args[0];
        if (aliases_.count(cmdName)) cmdName = aliases_[cmdName];
        
        if (commands_.count(cmdName)) {
            auto& cmd = commands_[cmdName];
            std::cout << std::endl;
            std::cout << colors::BOLD << "  /" << cmd.name << colors::RESET;
            if (!cmd.alias.empty()) {
                std::cout << colors::DIM << " (/" << cmd.alias << ")" << colors::RESET;
            }
            std::cout << std::endl;
            std::cout << "  " << cmd.description << std::endl;
            std::cout << std::endl;
            std::cout << colors::DIM << "  Usage: " << colors::RESET << cmd.usage << std::endl;
            std::cout << std::endl;
        } else {
            UI::error("Unknown command: " + cmdName);
        }
        return;
    }
    
    // Show all commands
    std::cout << std::endl;
    std::cout << colors::BOLD << "  Available Commands" << colors::RESET << std::endl;
    std::cout << std::endl;
    
    std::vector<std::tuple<std::string, std::string, std::string>> cmdList = {
        {"Testing", "/run, /list, /campaign", "Execute and manage attack techniques"},
        {"System", "/status, /snapshot, /clean", "System and EDR operations"},
        {"Config", "/config, /history", "Configuration and history"},
        {"General", "/help, /clear, /exit", "General commands"},
    };
    
    for (const auto& [category, cmds, desc] : cmdList) {
        std::cout << colors::CYAN << "  " << category << colors::RESET << std::endl;
        std::cout << "    " << colors::BRIGHT_CYAN << cmds << colors::RESET 
                  << colors::DIM << " - " << desc << colors::RESET << std::endl;
        std::cout << std::endl;
    }
    
    std::cout << colors::DIM << "  Type /help <command> for detailed help" << colors::RESET << std::endl;
    std::cout << std::endl;
}

void CLI::printPrompt() {
    std::cout << colors::BRIGHT_GREEN << "edr" << colors::RESET 
              << colors::DIM << "@" << colors::RESET
              << colors::BRIGHT_CYAN << "framework" << colors::RESET
              << colors::BRIGHT_YELLOW << " ❯ " << colors::RESET;
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
        if (token.substr(0, 2) == "--") {
            // Long option
            std::string key = token.substr(2);
            if (stream >> token && token[0] != '-') {
                ctx.options[key] = token;
            } else {
                ctx.options[key] = "true";
            }
        } else if (token[0] == '-' && token.length() == 2) {
            // Short option
            std::string key(1, token[1]);
            if (stream >> token && token[0] != '-') {
                ctx.options[key] = token;
            } else {
                ctx.options[key] = "true";
            }
        } else {
            ctx.args.push_back(token);
        }
    }
    
    ctx.verbose = ctx.options.count("verbose") || ctx.options.count("v");
    ctx.dryRun = ctx.options.count("dry-run");
    
    return ctx;
}

bool CLI::processInput(const std::string& input) {
    std::string trimmed = input;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
    
    if (trimmed.empty()) return true;
    
    // Add to history
    history_.push_back(trimmed);
    
    // Check if it's a slash command
    if (trimmed[0] == '/') {
        std::string cmdLine = trimmed.substr(1);
        auto ctx = parseInput(cmdLine);
        
        if (ctx.args.empty()) return true;
        
        std::string cmdName = ctx.args[0];
        ctx.args.erase(ctx.args.begin());
        
        // Check aliases
        if (aliases_.count(cmdName)) {
            cmdName = aliases_[cmdName];
        }
        
        if (commands_.count(cmdName)) {
            commands_[cmdName].handler(ctx);
        } else {
            UI::error("Unknown command: /" + cmdName);
            std::cout << colors::DIM << "  Type /help for available commands" 
                      << colors::RESET << std::endl;
        }
    } else {
        // Treat as a technique ID for quick run
        UI::info("Running technique: " + trimmed);
        CommandContext ctx;
        ctx.args.push_back(trimmed);
        cmdRun(ctx);
    }
    
    return true;
}

void CLI::runInteractive() {
    running_ = true;
    showWelcome();
    
    while (running_) {
        printPrompt();
        std::string input = readLine();
        
        if (std::cin.eof()) {
            running_ = false;
            break;
        }
        
        processInput(input);
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
        showHelp({});
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
        UI::error("No technique specified");
        std::cout << colors::DIM << "  Usage: /run <technique_id>" << colors::RESET << std::endl;
        return;
    }
    
    std::string techniqueId = ctx.args[0];
    
    std::cout << std::endl;
    UI::box("Executing Technique", techniqueId);
    std::cout << std::endl;
    
    // Simulate execution with progress
    UI::info("Preparing environment...");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    UI::info("Creating snapshot...");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    UI::info("Executing technique " + techniqueId + "...");
    for (int i = 0; i <= 100; i += 10) {
        UI::progress(i, 100, techniqueId);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
    
    std::vector<std::string> headers = {"ID", "Name", "Tactic", "Owner"};
    std::vector<std::vector<std::string>> data = {
        {"T1055", "Process Injection", "Defense Evasion", "Bipin"},
        {"T1055.012", "Process Hollowing", "Defense Evasion", "Bipin"},
        {"T1218.002", "Control Panel", "Defense Evasion", "Bipin"},
        {"T1218.005", "Mshta", "Defense Evasion", "Bipin"},
        {"T1574.002", "DLL Side-Loading", "Persistence", "Bipin"},
        {"T1106", "Direct Syscalls", "Execution", "Bipin"},
        {"T1562.001", "EDR Redirection", "Defense Evasion", "Bipin"},
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
        UI::error("No campaign file specified");
        std::cout << colors::DIM << "  Usage: /campaign <file.yaml>" << colors::RESET << std::endl;
        return;
    }
    
    std::cout << std::endl;
    UI::info("Loading campaign: " + ctx.args[0]);
    UI::warning("Campaign execution awaiting Bipin's exploit implementations");
    std::cout << std::endl;
}

void CLI::cmdSnapshot(const CommandContext& ctx) {
    std::string action = ctx.args.empty() ? "list" : ctx.args[0];
    
    std::cout << std::endl;
    
    if (action == "create") {
        std::string name = ctx.args.size() > 1 ? ctx.args[1] : "snapshot_" + currentSession_;
        UI::info("Creating snapshot: " + name);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        UI::success("Snapshot created");
    } else if (action == "restore") {
        if (ctx.args.size() < 2) {
            UI::error("Specify snapshot name to restore");
            return;
        }
        UI::info("Restoring snapshot: " + ctx.args[1]);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        UI::success("Snapshot restored");
    } else {
        UI::info("Available snapshots:");
        std::cout << colors::DIM << "  (No snapshots found)" << colors::RESET << std::endl;
    }
    
    std::cout << std::endl;
}

void CLI::cmdClean(const CommandContext& ctx) {
    std::cout << std::endl;
    
    if (ctx.options.count("all")) {
        if (UI::confirm("Remove ALL artifacts and restore system?")) {
            UI::info("Performing deep clean...");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            UI::success("All artifacts cleaned");
        }
    } else {
        UI::info("Cleaning session artifacts...");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        UI::success("Session cleaned");
    }
    
    std::cout << std::endl;
}

void CLI::cmdConfig(const CommandContext& ctx) {
    std::cout << std::endl;
    
    if (ctx.args.empty()) {
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
    } else {
        UI::info("Config key: " + ctx.args[0]);
        if (ctx.args.size() > 1) {
            UI::success("Set " + ctx.args[0] + " = " + ctx.args[1]);
        }
    }
    
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
