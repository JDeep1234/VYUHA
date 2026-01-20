/*
 * Integration Manager - Implementation
 * ======================================
 * Author: Jdeep
 */

#include "integration/integration_manager.hpp"
#include <iostream>

namespace edr {
namespace integration {

IntegrationManager::IntegrationManager() {
    std::cout << "[*] Integration Manager initialized" << std::endl;
}

IntegrationManager::~IntegrationManager() = default;

bool IntegrationManager::connectEDR(EDRType type, 
                                     const std::map<std::string, std::string>& config) {
    std::cout << "[*] Connecting to EDR..." << std::endl;
    
    switch (type) {
        case EDRType::CROWDSTRIKE: {
            auto connector = std::make_unique<CrowdStrikeConnector>(
                config.at("client_id"), 
                config.at("client_secret")
            );
            if (connector->connect()) {
                edrConnectors_.push_back(std::move(connector));
                return true;
            }
            break;
        }
        case EDRType::DEFENDER: {
            auto connector = std::make_unique<DefenderConnector>(
                config.at("tenant_id"),
                config.at("client_id"),
                config.at("client_secret")
            );
            if (connector->connect()) {
                edrConnectors_.push_back(std::move(connector));
                return true;
            }
            break;
        }
        case EDRType::CARBONBLACK: {
            auto connector = std::make_unique<CarbonBlackConnector>(
                config.at("api_url"),
                config.at("api_key"),
                config.at("org_key")
            );
            if (connector->connect()) {
                edrConnectors_.push_back(std::move(connector));
                return true;
            }
            break;
        }
        default:
            std::cerr << "[!] Unknown EDR type" << std::endl;
    }
    
    return false;
}

std::vector<EDRStatus> IntegrationManager::getActiveEDRs() {
    std::vector<EDRStatus> statuses;
    
    for (const auto& connector : edrConnectors_) {
        if (connector->isConnected()) {
            statuses.push_back(connector->getStatus());
        }
    }
    
    return statuses;
}

void IntegrationManager::getEDRStatus() {
    std::cout << "\n[*] EDR Status Check\n";
    std::cout << "==================\n";
    
    // Check local Defender
    if (DefenderConnector::isDefenderActive()) {
        std::cout << "[+] Windows Defender: ACTIVE\n";
        if (DefenderConnector::isRealTimeProtectionEnabled()) {
            std::cout << "    Real-time Protection: ENABLED\n";
        }
    }
    
    // Check connected EDRs
    for (const auto& connector : edrConnectors_) {
        auto status = connector->getStatus();
        std::cout << "[" << (status.active ? "+" : "-") << "] ";
        std::cout << status.name << ": " << (status.active ? "ACTIVE" : "INACTIVE") << "\n";
    }
}

std::vector<EDRType> IntegrationManager::detectInstalledEDRs() {
    std::vector<EDRType> detected;
    
    // Detection logic would check for EDR processes/services
    if (DefenderConnector::isDefenderActive()) {
        detected.push_back(EDRType::DEFENDER);
    }
    
    // Check for other EDRs...
    
    return detected;
}

bool IntegrationManager::createSnapshot(const std::string& name) {
    return snapshotManager_.createSnapshot(name, "Pre-test snapshot");
}

bool IntegrationManager::restoreSnapshot(const std::string& name) {
    return snapshotManager_.restoreSnapshot(name);
}

bool IntegrationManager::backupCurrentState() {
    std::cout << "[*] Backing up current system state..." << std::endl;
    return cleanModule_.backupRegistryKey("HKLM\\SOFTWARE\\EDRFramework");
}

bool IntegrationManager::restoreState() {
    return cleanModule_.restoreAll();
}

bool IntegrationManager::emergencyRestore() {
    std::cout << "[!] EMERGENCY RESTORE INITIATED" << std::endl;
    return cleanModule_.emergencyClean();
}

} // namespace integration
} // namespace edr
