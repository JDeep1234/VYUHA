/*
 * Integration Module - Header
 * ============================
 * Author: Jdeep
 * 
 * EDR integration, VM snapshots, and system restoration.
 */

#ifndef EDR_INTEGRATION_HPP
#define EDR_INTEGRATION_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace edr {
namespace integration {

// ============================================================================
// EDR CONNECTOR - Interface for EDR platforms
// ============================================================================

enum class EDRType {
    UNKNOWN,
    CROWDSTRIKE,
    DEFENDER,
    CARBONBLACK,
    SOPHOS,
    ESET,
    KASPERSKY,
    SENTINEL_ONE
};

struct EDRStatus {
    EDRType type;
    std::string name;
    std::string version;
    bool active;
    bool realTimeProtection;
    std::string lastUpdate;
};

class EDRConnector {
public:
    virtual ~EDRConnector() = default;
    
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual EDRStatus getStatus() = 0;
    virtual std::vector<std::string> getAlerts(int count = 100) = 0;
    virtual bool isConnected() const = 0;
};

// CrowdStrike Falcon Connector
class CrowdStrikeConnector : public EDRConnector {
public:
    CrowdStrikeConnector(const std::string& clientId, 
                         const std::string& clientSecret);
    ~CrowdStrikeConnector() override;
    
    bool connect() override;
    void disconnect() override;
    EDRStatus getStatus() override;
    std::vector<std::string> getAlerts(int count = 100) override;
    bool isConnected() const override;

private:
    std::string clientId_;
    std::string clientSecret_;
    std::string accessToken_;
    bool connected_;
};

// Microsoft Defender ATP Connector
class DefenderConnector : public EDRConnector {
public:
    DefenderConnector(const std::string& tenantId,
                      const std::string& clientId,
                      const std::string& clientSecret);
    ~DefenderConnector() override;
    
    bool connect() override;
    void disconnect() override;
    EDRStatus getStatus() override;
    std::vector<std::string> getAlerts(int count = 100) override;
    bool isConnected() const override;
    
    // Local Defender methods
    static bool isDefenderActive();
    static bool isRealTimeProtectionEnabled();

private:
    std::string tenantId_;
    std::string clientId_;
    std::string clientSecret_;
    std::string accessToken_;
    bool connected_;
};

// Carbon Black Connector
class CarbonBlackConnector : public EDRConnector {
public:
    CarbonBlackConnector(const std::string& apiUrl,
                         const std::string& apiKey,
                         const std::string& orgKey);
    ~CarbonBlackConnector() override;
    
    bool connect() override;
    void disconnect() override;
    EDRStatus getStatus() override;
    std::vector<std::string> getAlerts(int count = 100) override;
    bool isConnected() const override;

private:
    std::string apiUrl_;
    std::string apiKey_;
    std::string orgKey_;
    bool connected_;
};

// ============================================================================
// SNAPSHOT MANAGER - VM State Management
// ============================================================================

enum class VMProvider {
    AUTO,
    HYPERV,
    VIRTUALBOX,
    VMWARE
};

struct SnapshotInfo {
    std::string name;
    std::string description;
    std::string timestamp;
    size_t sizeBytes;
    VMProvider provider;
};

class SnapshotManager {
public:
    SnapshotManager();
    ~SnapshotManager();
    
    /**
     * Set VM provider
     */
    void setProvider(VMProvider provider);
    
    /**
     * Auto-detect VM provider
     */
    VMProvider detectProvider();
    
    /**
     * Create snapshot
     */
    bool createSnapshot(const std::string& name, 
                        const std::string& description = "");
    
    /**
     * Restore snapshot
     */
    bool restoreSnapshot(const std::string& name);
    
    /**
     * Delete snapshot
     */
    bool deleteSnapshot(const std::string& name);
    
    /**
     * List snapshots
     */
    std::vector<SnapshotInfo> listSnapshots();
    
    /**
     * Check if running in VM
     */
    bool isVirtualMachine() const;

private:
    VMProvider provider_;
    std::string vmName_;
    
    // Provider-specific implementations
    bool createHyperVSnapshot(const std::string& name);
    bool createVirtualBoxSnapshot(const std::string& name);
    bool createVMwareSnapshot(const std::string& name);
    
    bool restoreHyperVSnapshot(const std::string& name);
    bool restoreVirtualBoxSnapshot(const std::string& name);
    bool restoreVMwareSnapshot(const std::string& name);
};

// ============================================================================
// CLEAN MODULE - System Restoration
// ============================================================================

struct BackupEntry {
    std::string type;       // file, registry, service
    std::string path;
    std::string backupPath;
    std::string originalValue;
};

class CleanModule {
public:
    CleanModule();
    ~CleanModule();
    
    /**
     * Backup registry key
     */
    bool backupRegistryKey(const std::string& keyPath);
    
    /**
     * Backup file
     */
    bool backupFile(const std::string& filePath);
    
    /**
     * Restore all backups
     */
    bool restoreAll();
    
    /**
     * Restore specific backup
     */
    bool restore(const std::string& path);
    
    /**
     * Emergency cleanup - restore all and delete artifacts
     */
    bool emergencyClean();
    
    /**
     * Clear all backups
     */
    void clearBackups();

private:
    std::vector<BackupEntry> backups_;
    std::string backupDir_;
    
    bool restoreRegistryKey(const BackupEntry& entry);
    bool restoreFile(const BackupEntry& entry);
};

// ============================================================================
// INTEGRATION MANAGER - Central Coordinator
// ============================================================================

class IntegrationManager {
public:
    IntegrationManager();
    ~IntegrationManager();
    
    /**
     * Connect to EDR platform
     */
    bool connectEDR(EDRType type, const std::map<std::string, std::string>& config);
    
    /**
     * Get active EDR connectors
     */
    std::vector<EDRStatus> getActiveEDRs();
    
    /**
     * Get EDR status
     */
    void getEDRStatus();
    
    /**
     * Auto-detect installed EDRs
     */
    std::vector<EDRType> detectInstalledEDRs();
    
    /**
     * Snapshot operations
     */
    bool createSnapshot(const std::string& name);
    bool restoreSnapshot(const std::string& name);
    
    /**
     * Backup and restore
     */
    bool backupCurrentState();
    bool restoreState();
    
    /**
     * Emergency cleanup
     */
    bool emergencyRestore();

private:
    std::vector<std::unique_ptr<EDRConnector>> edrConnectors_;
    SnapshotManager snapshotManager_;
    CleanModule cleanModule_;
};

} // namespace integration
} // namespace edr

#endif // EDR_INTEGRATION_HPP
