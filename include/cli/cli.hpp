/*
 * Interactive CLI - Header
 * =========================
 * Author: Jdeep
 * 
 * Claude-style interactive terminal interface.
 * Features: Colors, slash commands, interactive mode, real-time output.
 */

#ifndef EDR_CLI_HPP
#define EDR_CLI_HPP

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

// Forward declarations
namespace edr {
namespace exploits {
    class ExploitManager;
}
namespace agent {
    class AgentCore;
}
namespace ml {
    class MLEngine;
}
}

namespace edr {
namespace cli {

// ============================================================================
// ANSI COLOR CODES
// ============================================================================

namespace colors {
    // Reset
    constexpr const char* RESET = "\033[0m";
    
    // Regular colors
    constexpr const char* BLACK = "\033[30m";
    constexpr const char* RED = "\033[31m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* BLUE = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN = "\033[36m";
    constexpr const char* WHITE = "\033[37m";
    
    // Bright colors
    constexpr const char* BRIGHT_RED = "\033[91m";
    constexpr const char* BRIGHT_GREEN = "\033[92m";
    constexpr const char* BRIGHT_YELLOW = "\033[93m";
    constexpr const char* BRIGHT_BLUE = "\033[94m";
    constexpr const char* BRIGHT_MAGENTA = "\033[95m";
    constexpr const char* BRIGHT_CYAN = "\033[96m";
    
    // Styles
    constexpr const char* BOLD = "\033[1m";
    constexpr const char* DIM = "\033[2m";
    constexpr const char* ITALIC = "\033[3m";
    constexpr const char* UNDERLINE = "\033[4m";
    
    // Background
    constexpr const char* BG_RED = "\033[41m";
    constexpr const char* BG_GREEN = "\033[42m";
    constexpr const char* BG_YELLOW = "\033[43m";
    constexpr const char* BG_BLUE = "\033[44m";
}

// ============================================================================
// UI COMPONENTS
// ============================================================================

class UI {
public:
    // Print colored text
    static void print(const std::string& text, const char* color = colors::RESET);
    static void println(const std::string& text, const char* color = colors::RESET);
    
    // Print with prefix icons
    static void info(const std::string& text);
    static void success(const std::string& text);
    static void warning(const std::string& text);
    static void error(const std::string& text);
    static void debug(const std::string& text);
    
    // Boxes and panels
    static void box(const std::string& title, const std::string& content);
    static void panel(const std::string& text, const char* borderColor = colors::CYAN);
    static void divider(const char* color = colors::DIM);
    
    // Progress indicators
    static void spinner(const std::string& text);
    static void progress(int current, int total, const std::string& label = "");
    
    // Tables
    static void table(const std::vector<std::vector<std::string>>& data, 
                      const std::vector<std::string>& headers = {});
    
    // Input
    static std::string prompt(const std::string& message);
    static bool confirm(const std::string& message);
    
    // Clear screen
    static void clear();
    
    // Banner
    static void banner();
    
    // Enable Windows ANSI
    static void enableAnsi();
};

// ============================================================================
// COMMAND STRUCTURE
// ============================================================================

struct CommandContext {
    std::vector<std::string> args;
    std::map<std::string, std::string> options;
    bool verbose = false;
    bool dryRun = false;
};

using CommandHandler = std::function<void(const CommandContext&)>;

struct Command {
    std::string name;
    std::string alias;
    std::string description;
    std::string usage;
    CommandHandler handler;
};

// ============================================================================
// INTERACTIVE CLI
// ============================================================================

class CLI {
public:
    CLI();
    ~CLI();
    
    /**
     * Run in interactive mode (Claude-style REPL)
     */
    void runInteractive();
    
    /**
     * Run single command from arguments
     */
    int runCommand(int argc, char* argv[]);
    
    /**
     * Register a command
     */
    void registerCommand(const Command& cmd);
    
    /**
     * Process a single input line
     */
    bool processInput(const std::string& input);

private:
    std::map<std::string, Command> commands_;
    std::map<std::string, std::string> aliases_;
    std::vector<std::string> history_;
    bool running_;
    std::string currentSession_;
    std::shared_ptr<edr::exploits::ExploitManager> exploitManager_;
    std::shared_ptr<edr::agent::AgentCore>          agentCore_;
    std::shared_ptr<edr::ml::MLEngine>               mlEngine_;
    
    void registerBuiltinCommands();
    void showHelp(const CommandContext& ctx);
    void showWelcome();
    void printPrompt();
    std::string readLine();
    CommandContext parseInput(const std::string& input);
    
    // Menu handlers
    void showMainMenu();
    void showTechniqueMenu();
    void showSnapshotMenu();
    void handleMainMenuChoice(int choice);
    void handleTechniqueMenu();
    void handleCampaignMenu();
    void handleSnapshotMenu();
    
    // Built-in command handlers
    void cmdRun(const CommandContext& ctx);
    void cmdList(const CommandContext& ctx);
    void cmdStatus(const CommandContext& ctx);
    void cmdCampaign(const CommandContext& ctx);
    void cmdSnapshot(const CommandContext& ctx);
    void cmdClean(const CommandContext& ctx);
    void cmdConfig(const CommandContext& ctx);
    void cmdHistory(const CommandContext& ctx);
    void cmdClear(const CommandContext& ctx);
    void cmdExit(const CommandContext& ctx);
};

} // namespace cli
} // namespace edr

#endif // EDR_CLI_HPP
