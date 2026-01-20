/*
 * Snapshot Manager - Implementation
 * ===================================
 * Author: Jdeep
 */

#include "integration/integration_manager.hpp"
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

namespace edr {
namespace integration {

SnapshotManager::SnapshotManager()
    : provider_(VMProvider::AUTO) {
    // Auto-detect provider on initialization
    provider_ = detectProvider();
}

SnapshotManager::~SnapshotManager() = default;

void SnapshotManager::setProvider(VMProvider provider) {
    provider_ = provider;
    std::cout << "[*] VM Provider set to: ";
    switch (provider) {
        case VMProvider::HYPERV: std::cout << "Hyper-V"; break;
        case VMProvider::VIRTUALBOX: std::cout << "VirtualBox"; break;
        case VMProvider::VMWARE: std::cout << "VMware"; break;
        default: std::cout << "Auto"; break;
    }
    std::cout << std::endl;
}

VMProvider SnapshotManager::detectProvider() {
#ifdef _WIN32
    // Check for Hyper-V
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (scm) {
        SC_HANDLE svc = OpenService(scm, L"vmms", SERVICE_QUERY_STATUS);
        if (svc) {
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            std::cout << "[*] Detected: Hyper-V" << std::endl;
            return VMProvider::HYPERV;
        }
        CloseServiceHandle(scm);
    }
    
    // Check for VirtualBox
    if (GetModuleHandleA("VBoxService.exe") || 
        system("where VBoxManage >nul 2>&1") == 0) {
        std::cout << "[*] Detected: VirtualBox" << std::endl;
        return VMProvider::VIRTUALBOX;
    }
    
    // Check for VMware
    if (GetModuleHandleA("vmtoolsd.exe") ||
        system("where vmrun >nul 2>&1") == 0) {
        std::cout << "[*] Detected: VMware" << std::endl;
        return VMProvider::VMWARE;
    }
#endif
    
    std::cout << "[*] No VM provider detected" << std::endl;
    return VMProvider::AUTO;
}

bool SnapshotManager::isVirtualMachine() const {
#ifdef _WIN32
    // Check for VM indicators
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Virtual Machine\\Guest\\Parameters",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    
    // Check BIOS for VM strings
    // Additional checks could include CPUID, registry, etc.
#endif
    return false;
}

bool SnapshotManager::createSnapshot(const std::string& name,
                                      const std::string& description) {
    std::cout << "[*] Creating snapshot: " << name << std::endl;
    
    switch (provider_) {
        case VMProvider::HYPERV:
            return createHyperVSnapshot(name);
        case VMProvider::VIRTUALBOX:
            return createVirtualBoxSnapshot(name);
        case VMProvider::VMWARE:
            return createVMwareSnapshot(name);
        default:
            std::cerr << "[!] No VM provider configured" << std::endl;
            return false;
    }
}

bool SnapshotManager::restoreSnapshot(const std::string& name) {
    std::cout << "[*] Restoring snapshot: " << name << std::endl;
    
    switch (provider_) {
        case VMProvider::HYPERV:
            return restoreHyperVSnapshot(name);
        case VMProvider::VIRTUALBOX:
            return restoreVirtualBoxSnapshot(name);
        case VMProvider::VMWARE:
            return restoreVMwareSnapshot(name);
        default:
            return false;
    }
}

bool SnapshotManager::deleteSnapshot(const std::string& name) {
    std::cout << "[*] Deleting snapshot: " << name << std::endl;
    // Implementation per provider
    return true;
}

std::vector<SnapshotInfo> SnapshotManager::listSnapshots() {
    std::vector<SnapshotInfo> snapshots;
    // Query provider for snapshots
    return snapshots;
}

// Hyper-V implementations
bool SnapshotManager::createHyperVSnapshot(const std::string& name) {
    std::string cmd = "powershell -Command \"Checkpoint-VM -Name '" + vmName_ + 
                      "' -SnapshotName '" + name + "'\"";
    std::cout << "[*] Executing: " << cmd << std::endl;
    return system(cmd.c_str()) == 0;
}

bool SnapshotManager::restoreHyperVSnapshot(const std::string& name) {
    std::string cmd = "powershell -Command \"Restore-VMSnapshot -VMName '" + vmName_ + 
                      "' -Name '" + name + "' -Confirm:$false\"";
    return system(cmd.c_str()) == 0;
}

// VirtualBox implementations
bool SnapshotManager::createVirtualBoxSnapshot(const std::string& name) {
    std::string cmd = "VBoxManage snapshot \"" + vmName_ + "\" take \"" + name + "\"";
    return system(cmd.c_str()) == 0;
}

bool SnapshotManager::restoreVirtualBoxSnapshot(const std::string& name) {
    std::string cmd = "VBoxManage snapshot \"" + vmName_ + "\" restore \"" + name + "\"";
    return system(cmd.c_str()) == 0;
}

// VMware implementations
bool SnapshotManager::createVMwareSnapshot(const std::string& name) {
    std::string cmd = "vmrun snapshot \"" + vmName_ + "\" \"" + name + "\"";
    return system(cmd.c_str()) == 0;
}

bool SnapshotManager::restoreVMwareSnapshot(const std::string& name) {
    std::string cmd = "vmrun revertToSnapshot \"" + vmName_ + "\" \"" + name + "\"";
    return system(cmd.c_str()) == 0;
}

} // namespace integration
} // namespace edr
