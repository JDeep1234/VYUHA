"""
Integration Manager
===================

Central manager for all integration components.
Coordinates EDR connectors, snapshot management, and system restoration.
"""

import logging
from typing import Dict, Any, List, Optional

from jdeep.integration.edr_connector import EDRConnector
from jdeep.integration.snapshot_manager import SnapshotManager
from jdeep.integration.clean_module import CleanModule

logger = logging.getLogger(__name__)


class IntegrationManager:
    """
    Central integration manager.
    
    Coordinates:
    - EDR API connections
    - VM snapshot management
    - System state restoration
    """
    
    def __init__(self, config=None):
        """
        Initialize the Integration Manager.
        
        Args:
            config: ConfigManager instance
        """
        self.config = config
        
        # Initialize components
        self.edr_connector = EDRConnector(config)
        self.snapshot_manager = SnapshotManager(config)
        self.clean_module = CleanModule(config)
        
        # Track connected EDRs
        self.connected_edrs: List[str] = []
        
        logger.info("IntegrationManager initialized")
    
    def connect_edr(self, edr_name: str, credentials: Dict[str, Any]) -> bool:
        """
        Connect to an EDR platform.
        
        Args:
            edr_name: Name of EDR (crowdstrike, defender, carbonblack)
            credentials: API credentials
            
        Returns:
            True if connection successful
        """
        success = self.edr_connector.connect(edr_name, credentials)
        if success:
            self.connected_edrs.append(edr_name)
        return success
    
    def get_edr_status(self) -> List[Dict[str, Any]]:
        """
        Get status of all EDR/AV systems.
        
        Returns:
            List of EDR status dictionaries
        """
        return self.edr_connector.get_all_status()
    
    def get_edr_alerts(
        self,
        edr_name: Optional[str] = None,
        time_range: Optional[int] = 3600
    ) -> List[Dict[str, Any]]:
        """
        Get alerts from EDR platforms.
        
        Args:
            edr_name: Specific EDR or None for all
            time_range: Time range in seconds
            
        Returns:
            List of alert dictionaries
        """
        return self.edr_connector.get_alerts(edr_name, time_range)
    
    def create_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """
        Create a VM snapshot.
        
        Args:
            vm_name: Virtual machine name
            snapshot_name: Snapshot name
            
        Returns:
            True if successful
        """
        return self.snapshot_manager.create_snapshot(vm_name, snapshot_name)
    
    def restore_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """
        Restore a VM to a snapshot.
        
        Args:
            vm_name: Virtual machine name
            snapshot_name: Snapshot name
            
        Returns:
            True if successful
        """
        return self.snapshot_manager.restore_snapshot(vm_name, snapshot_name)
    
    def prepare_test_environment(self, vm_name: str) -> Dict[str, Any]:
        """
        Prepare clean test environment.
        
        Args:
            vm_name: Virtual machine name
            
        Returns:
            Environment status dictionary
        """
        result = {
            "vm_name": vm_name,
            "snapshot_created": False,
            "environment_ready": False,
            "errors": []
        }
        
        try:
            # Create pre-test snapshot
            snapshot_name = f"pre_test_{vm_name}"
            if self.snapshot_manager.create_snapshot(vm_name, snapshot_name):
                result["snapshot_created"] = True
                result["snapshot_name"] = snapshot_name
            
            result["environment_ready"] = True
            
        except Exception as e:
            result["errors"].append(str(e))
            logger.error(f"Failed to prepare environment: {e}")
        
        return result
    
    def restore_test_environment(self, vm_name: str, snapshot_name: str) -> Dict[str, Any]:
        """
        Restore test environment to clean state.
        
        Args:
            vm_name: Virtual machine name
            snapshot_name: Snapshot to restore
            
        Returns:
            Restoration status dictionary
        """
        result = {
            "vm_name": vm_name,
            "snapshot_restored": False,
            "cleaned": False,
            "errors": []
        }
        
        try:
            # Restore snapshot
            if self.snapshot_manager.restore_snapshot(vm_name, snapshot_name):
                result["snapshot_restored"] = True
            
            # Additional cleanup
            self.clean_module.full_clean()
            result["cleaned"] = True
            
        except Exception as e:
            result["errors"].append(str(e))
            logger.error(f"Failed to restore environment: {e}")
        
        return result
