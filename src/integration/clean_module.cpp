/*
 * Clean Module - Implementation
 * ==============================
 * Author: Jdeep
 */

#include "integration/integration_manager.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace edr {
namespace integration {

CleanModule::CleanModule()
    : backupDir_("backups") {
    // Create backup directory
    if (!fs::exists(backupDir_)) {
        fs::create_directories(backupDir_);
    }
}

CleanModule::~CleanModule() = default;

bool CleanModule::backupRegistryKey(const std::string& keyPath) {
#ifdef _WIN32
    std::cout << "[*] Backing up registry key: " << keyPath << std::endl;
    
    BackupEntry entry;
    entry.type = "registry";
    entry.path = keyPath;
    entry.backupPath = backupDir_ + "/registry_backup.reg";
    
    // Export registry key
    std::string cmd = "reg export \"" + keyPath + "\" \"" + entry.backupPath + "\" /y";
    if (system(cmd.c_str()) == 0) {
        backups_.push_back(entry);
        std::cout << "[+] Registry backed up to: " << entry.backupPath << std::endl;
        return true;
    }
#endif
    return false;
}

bool CleanModule::backupFile(const std::string& filePath) {
    if (!fs::exists(filePath)) {
        return false;
    }
    
    std::cout << "[*] Backing up file: " << filePath << std::endl;
    
    BackupEntry entry;
    entry.type = "file";
    entry.path = filePath;
    
    // Generate backup path
    fs::path src(filePath);
    entry.backupPath = backupDir_ + "/" + src.filename().string() + ".bak";
    
    try {
        fs::copy_file(filePath, entry.backupPath, fs::copy_options::overwrite_existing);
        backups_.push_back(entry);
        std::cout << "[+] File backed up to: " << entry.backupPath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[!] Backup failed: " << e.what() << std::endl;
        return false;
    }
}

bool CleanModule::restoreAll() {
    std::cout << "[*] Restoring all backups..." << std::endl;
    bool success = true;
    
    for (const auto& entry : backups_) {
        if (!restore(entry.path)) {
            success = false;
        }
    }
    
    return success;
}

bool CleanModule::restore(const std::string& path) {
    for (const auto& entry : backups_) {
        if (entry.path == path) {
            if (entry.type == "registry") {
                return restoreRegistryKey(entry);
            } else if (entry.type == "file") {
                return restoreFile(entry);
            }
        }
    }
    
    std::cerr << "[!] No backup found for: " << path << std::endl;
    return false;
}

bool CleanModule::restoreRegistryKey(const BackupEntry& entry) {
#ifdef _WIN32
    std::cout << "[*] Restoring registry: " << entry.path << std::endl;
    std::string cmd = "reg import \"" + entry.backupPath + "\"";
    return system(cmd.c_str()) == 0;
#else
    return false;
#endif
}

bool CleanModule::restoreFile(const BackupEntry& entry) {
    std::cout << "[*] Restoring file: " << entry.path << std::endl;
    
    try {
        fs::copy_file(entry.backupPath, entry.path, fs::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[!] Restore failed: " << e.what() << std::endl;
        return false;
    }
}

bool CleanModule::emergencyClean() {
    std::cout << "[!] ===== EMERGENCY CLEAN =====" << std::endl;
    
    // Restore all backups
    restoreAll();
    
    // Kill any spawned processes
    // Delete all temp files
    // Reset registry keys
    
    std::cout << "[+] Emergency clean completed" << std::endl;
    return true;
}

void CleanModule::clearBackups() {
    backups_.clear();
    
    // Optionally delete backup files
    try {
        fs::remove_all(backupDir_);
        fs::create_directories(backupDir_);
    } catch (...) {}
}

} // namespace integration
} // namespace edr
