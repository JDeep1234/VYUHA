"""
Snapshot Manager Module
=======================

Manages VM snapshots for testing environments.
Supports VMware vSphere, Hyper-V, and VirtualBox.
"""

import logging
import subprocess
from typing import Dict, Any, List, Optional
from datetime import datetime
from abc import ABC, abstractmethod

logger = logging.getLogger(__name__)


class BaseVMProvider(ABC):
    """Abstract base class for VM providers."""
    
    @abstractmethod
    def create_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        pass
    
    @abstractmethod
    def restore_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        pass
    
    @abstractmethod
    def delete_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        pass
    
    @abstractmethod
    def list_snapshots(self, vm_name: Optional[str] = None) -> List[Dict[str, Any]]:
        pass


class HyperVProvider(BaseVMProvider):
    """Hyper-V snapshot provider."""
    
    def create_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """Create a Hyper-V checkpoint."""
        try:
            result = subprocess.run(
                ["powershell", "-Command",
                 f"Checkpoint-VM -Name '{vm_name}' -SnapshotName '{snapshot_name}'"],
                capture_output=True,
                text=True
            )
            
            if result.returncode == 0:
                logger.info(f"Created Hyper-V snapshot: {snapshot_name} for {vm_name}")
                return True
            else:
                logger.error(f"Failed to create snapshot: {result.stderr}")
                return False
                
        except Exception as e:
            logger.error(f"Hyper-V snapshot error: {e}")
            return False
    
    def restore_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """Restore a Hyper-V checkpoint."""
        try:
            result = subprocess.run(
                ["powershell", "-Command",
                 f"Restore-VMSnapshot -VMName '{vm_name}' -Name '{snapshot_name}' -Confirm:$false"],
                capture_output=True,
                text=True
            )
            
            if result.returncode == 0:
                logger.info(f"Restored Hyper-V snapshot: {snapshot_name} for {vm_name}")
                return True
            else:
                logger.error(f"Failed to restore snapshot: {result.stderr}")
                return False
                
        except Exception as e:
            logger.error(f"Hyper-V restore error: {e}")
            return False
    
    def delete_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """Delete a Hyper-V checkpoint."""
        try:
            result = subprocess.run(
                ["powershell", "-Command",
                 f"Remove-VMSnapshot -VMName '{vm_name}' -Name '{snapshot_name}' -Confirm:$false"],
                capture_output=True,
                text=True
            )
            
            return result.returncode == 0
            
        except Exception as e:
            logger.error(f"Hyper-V delete error: {e}")
            return False
    
    def list_snapshots(self, vm_name: Optional[str] = None) -> List[Dict[str, Any]]:
        """List Hyper-V checkpoints."""
        try:
            cmd = "Get-VMSnapshot"
            if vm_name:
                cmd += f" -VMName '{vm_name}'"
            cmd += " | Select-Object VMName, Name, CreationTime | ConvertTo-Json"
            
            result = subprocess.run(
                ["powershell", "-Command", cmd],
                capture_output=True,
                text=True
            )
            
            if result.returncode == 0 and result.stdout.strip():
                import json
                data = json.loads(result.stdout)
                
                # Handle single vs multiple results
                if isinstance(data, dict):
                    data = [data]
                
                return [
                    {
                        "vm": s.get("VMName"),
                        "name": s.get("Name"),
                        "created": s.get("CreationTime")
                    }
                    for s in data
                ]
            return []
            
        except Exception as e:
            logger.error(f"Hyper-V list error: {e}")
            return []


class VirtualBoxProvider(BaseVMProvider):
    """VirtualBox snapshot provider."""
    
    def __init__(self):
        self.vboxmanage = "VBoxManage"
    
    def create_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """Create a VirtualBox snapshot."""
        try:
            result = subprocess.run(
                [self.vboxmanage, "snapshot", vm_name, "take", snapshot_name],
                capture_output=True,
                text=True
            )
            
            if result.returncode == 0:
                logger.info(f"Created VirtualBox snapshot: {snapshot_name} for {vm_name}")
                return True
            else:
                logger.error(f"Failed to create snapshot: {result.stderr}")
                return False
                
        except Exception as e:
            logger.error(f"VirtualBox snapshot error: {e}")
            return False
    
    def restore_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """Restore a VirtualBox snapshot."""
        try:
            result = subprocess.run(
                [self.vboxmanage, "snapshot", vm_name, "restore", snapshot_name],
                capture_output=True,
                text=True
            )
            
            if result.returncode == 0:
                logger.info(f"Restored VirtualBox snapshot: {snapshot_name} for {vm_name}")
                return True
            else:
                logger.error(f"Failed to restore snapshot: {result.stderr}")
                return False
                
        except Exception as e:
            logger.error(f"VirtualBox restore error: {e}")
            return False
    
    def delete_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """Delete a VirtualBox snapshot."""
        try:
            result = subprocess.run(
                [self.vboxmanage, "snapshot", vm_name, "delete", snapshot_name],
                capture_output=True,
                text=True
            )
            
            return result.returncode == 0
            
        except Exception as e:
            logger.error(f"VirtualBox delete error: {e}")
            return False
    
    def list_snapshots(self, vm_name: Optional[str] = None) -> List[Dict[str, Any]]:
        """List VirtualBox snapshots."""
        snapshots = []
        
        try:
            # Get list of VMs
            if vm_name:
                vms = [vm_name]
            else:
                result = subprocess.run(
                    [self.vboxmanage, "list", "vms"],
                    capture_output=True,
                    text=True
                )
                vms = [line.split('"')[1] for line in result.stdout.splitlines() if '"' in line]
            
            # Get snapshots for each VM
            for vm in vms:
                result = subprocess.run(
                    [self.vboxmanage, "snapshot", vm, "list", "--machinereadable"],
                    capture_output=True,
                    text=True
                )
                
                for line in result.stdout.splitlines():
                    if line.startswith("SnapshotName"):
                        name = line.split("=")[1].strip('"')
                        snapshots.append({
                            "vm": vm,
                            "name": name,
                            "created": "N/A"
                        })
                        
        except Exception as e:
            logger.error(f"VirtualBox list error: {e}")
        
        return snapshots


class VMwareProvider(BaseVMProvider):
    """VMware vSphere/Workstation snapshot provider."""
    
    def __init__(self, config=None):
        self.config = config
        self.vmrun = r"C:\Program Files (x86)\VMware\VMware Workstation\vmrun.exe"
    
    def create_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """Create a VMware snapshot."""
        try:
            result = subprocess.run(
                [self.vmrun, "snapshot", vm_name, snapshot_name],
                capture_output=True,
                text=True
            )
            
            if result.returncode == 0:
                logger.info(f"Created VMware snapshot: {snapshot_name}")
                return True
            else:
                logger.error(f"Failed to create snapshot: {result.stderr}")
                return False
                
        except Exception as e:
            logger.error(f"VMware snapshot error: {e}")
            return False
    
    def restore_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """Restore a VMware snapshot."""
        try:
            result = subprocess.run(
                [self.vmrun, "revertToSnapshot", vm_name, snapshot_name],
                capture_output=True,
                text=True
            )
            
            if result.returncode == 0:
                logger.info(f"Restored VMware snapshot: {snapshot_name}")
                return True
            else:
                logger.error(f"Failed to restore snapshot: {result.stderr}")
                return False
                
        except Exception as e:
            logger.error(f"VMware restore error: {e}")
            return False
    
    def delete_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """Delete a VMware snapshot."""
        try:
            result = subprocess.run(
                [self.vmrun, "deleteSnapshot", vm_name, snapshot_name],
                capture_output=True,
                text=True
            )
            
            return result.returncode == 0
            
        except Exception as e:
            logger.error(f"VMware delete error: {e}")
            return False
    
    def list_snapshots(self, vm_name: Optional[str] = None) -> List[Dict[str, Any]]:
        """List VMware snapshots."""
        if not vm_name:
            return []
        
        try:
            result = subprocess.run(
                [self.vmrun, "listSnapshots", vm_name],
                capture_output=True,
                text=True
            )
            
            snapshots = []
            for line in result.stdout.splitlines()[1:]:  # Skip header
                if line.strip():
                    snapshots.append({
                        "vm": vm_name,
                        "name": line.strip(),
                        "created": "N/A"
                    })
            
            return snapshots
            
        except Exception as e:
            logger.error(f"VMware list error: {e}")
            return []


class SnapshotManager:
    """
    Main snapshot manager class.
    
    Manages VM snapshots across different providers.
    """
    
    PROVIDERS = {
        "hyperv": HyperVProvider,
        "virtualbox": VirtualBoxProvider,
        "vmware": VMwareProvider,
    }
    
    def __init__(self, config=None):
        """
        Initialize Snapshot Manager.
        
        Args:
            config: ConfigManager instance
        """
        self.config = config
        self.provider_name = self._detect_provider()
        self.provider = self._get_provider()
        
        logger.info(f"SnapshotManager initialized with provider: {self.provider_name}")
    
    def _detect_provider(self) -> str:
        """Detect available VM provider."""
        # Check for Hyper-V
        try:
            result = subprocess.run(
                ["powershell", "-Command", "Get-VM"],
                capture_output=True,
                text=True
            )
            if result.returncode == 0:
                return "hyperv"
        except:
            pass
        
        # Check for VirtualBox
        try:
            result = subprocess.run(
                ["VBoxManage", "--version"],
                capture_output=True,
                text=True
            )
            if result.returncode == 0:
                return "virtualbox"
        except:
            pass
        
        # Check for VMware
        vmrun = r"C:\Program Files (x86)\VMware\VMware Workstation\vmrun.exe"
        try:
            result = subprocess.run(
                [vmrun, "-v"],
                capture_output=True,
                text=True
            )
            if result.returncode == 0:
                return "vmware"
        except:
            pass
        
        return "hyperv"  # Default
    
    def _get_provider(self) -> BaseVMProvider:
        """Get the VM provider instance."""
        provider_class = self.PROVIDERS.get(self.provider_name, HyperVProvider)
        return provider_class()
    
    def set_provider(self, provider_name: str):
        """
        Set the VM provider.
        
        Args:
            provider_name: hyperv, virtualbox, or vmware
        """
        if provider_name.lower() in self.PROVIDERS:
            self.provider_name = provider_name.lower()
            self.provider = self._get_provider()
            logger.info(f"Switched to provider: {provider_name}")
    
    def create_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """Create a snapshot."""
        return self.provider.create_snapshot(vm_name, snapshot_name)
    
    def restore_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """Restore a snapshot."""
        return self.provider.restore_snapshot(vm_name, snapshot_name)
    
    def delete_snapshot(self, vm_name: str, snapshot_name: str) -> bool:
        """Delete a snapshot."""
        return self.provider.delete_snapshot(vm_name, snapshot_name)
    
    def list_snapshots(self, vm_name: Optional[str] = None) -> List[Dict[str, Any]]:
        """List snapshots."""
        return self.provider.list_snapshots(vm_name)
