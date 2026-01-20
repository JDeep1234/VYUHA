"""
Clean Module
============

Handles system restoration and cleanup after testing.
Restores system to clean state by reversing test modifications.
"""

import os
import logging
import subprocess
import winreg
from typing import Dict, Any, List, Optional
from pathlib import Path

logger = logging.getLogger(__name__)


class CleanModule:
    """
    Restores system to clean state after testing.
    
    Handles:
    - Registry restoration
    - File cleanup
    - Service removal
    - Task removal
    - Process termination
    """
    
    def __init__(self, config=None):
        """
        Initialize Clean Module.
        
        Args:
            config: ConfigManager instance
        """
        self.config = config
        
        # Track modifications for restoration
        self.registry_backups: List[Dict[str, Any]] = []
        self.file_backups: Dict[str, str] = {}  # original_path -> backup_path
        self.created_files: List[str] = []
        self.created_services: List[str] = []
        self.created_tasks: List[str] = []
        
        logger.info("CleanModule initialized")
    
    def backup_registry_key(self, hive: int, key_path: str, value_name: str):
        """
        Backup a registry value before modification.
        
        Args:
            hive: Registry hive (e.g., winreg.HKEY_LOCAL_MACHINE)
            key_path: Registry key path
            value_name: Value name to backup
        """
        try:
            with winreg.OpenKey(hive, key_path, 0, winreg.KEY_READ) as key:
                value, reg_type = winreg.QueryValueEx(key, value_name)
                
                self.registry_backups.append({
                    "hive": hive,
                    "key": key_path,
                    "value_name": value_name,
                    "value": value,
                    "type": reg_type,
                    "existed": True
                })
                
        except FileNotFoundError:
            # Value doesn't exist
            self.registry_backups.append({
                "hive": hive,
                "key": key_path,
                "value_name": value_name,
                "existed": False
            })
        except Exception as e:
            logger.error(f"Failed to backup registry: {e}")
    
    def backup_file(self, file_path: str) -> Optional[str]:
        """
        Backup a file before modification.
        
        Args:
            file_path: Path to file
            
        Returns:
            Backup path or None
        """
        try:
            path = Path(file_path)
            if path.exists():
                backup_path = path.with_suffix(path.suffix + ".edr_backup")
                import shutil
                shutil.copy2(path, backup_path)
                
                self.file_backups[file_path] = str(backup_path)
                logger.debug(f"Backed up file: {file_path}")
                return str(backup_path)
                
        except Exception as e:
            logger.error(f"Failed to backup file: {e}")
        
        return None
    
    def track_created_file(self, file_path: str):
        """Track a file created during testing."""
        self.created_files.append(file_path)
    
    def track_created_service(self, service_name: str):
        """Track a service created during testing."""
        self.created_services.append(service_name)
    
    def track_created_task(self, task_name: str):
        """Track a scheduled task created during testing."""
        self.created_tasks.append(task_name)
    
    def restore_registry(self) -> int:
        """
        Restore all backed up registry values.
        
        Returns:
            Number of values restored
        """
        restored = 0
        
        for backup in self.registry_backups[:]:
            try:
                if backup["existed"]:
                    # Restore original value
                    with winreg.OpenKey(
                        backup["hive"], 
                        backup["key"], 
                        0, 
                        winreg.KEY_SET_VALUE
                    ) as key:
                        winreg.SetValueEx(
                            key,
                            backup["value_name"],
                            0,
                            backup["type"],
                            backup["value"]
                        )
                else:
                    # Delete the value (it didn't exist before)
                    with winreg.OpenKey(
                        backup["hive"],
                        backup["key"],
                        0,
                        winreg.KEY_SET_VALUE
                    ) as key:
                        try:
                            winreg.DeleteValue(key, backup["value_name"])
                        except FileNotFoundError:
                            pass
                
                restored += 1
                self.registry_backups.remove(backup)
                
            except Exception as e:
                logger.error(f"Failed to restore registry: {e}")
        
        return restored
    
    def restore_files(self) -> int:
        """
        Restore all backed up files.
        
        Returns:
            Number of files restored
        """
        restored = 0
        
        for original_path, backup_path in list(self.file_backups.items()):
            try:
                import shutil
                shutil.copy2(backup_path, original_path)
                Path(backup_path).unlink()  # Remove backup
                
                restored += 1
                del self.file_backups[original_path]
                
            except Exception as e:
                logger.error(f"Failed to restore file: {e}")
        
        return restored
    
    def clean_created_files(self) -> int:
        """
        Remove all files created during testing.
        
        Returns:
            Number of files removed
        """
        removed = 0
        
        for file_path in self.created_files[:]:
            try:
                path = Path(file_path)
                if path.exists():
                    if path.is_file():
                        path.unlink()
                    elif path.is_dir():
                        import shutil
                        shutil.rmtree(path)
                    removed += 1
                
                self.created_files.remove(file_path)
                
            except Exception as e:
                logger.error(f"Failed to remove file {file_path}: {e}")
        
        return removed
    
    def clean_created_services(self) -> int:
        """
        Remove all services created during testing.
        
        Returns:
            Number of services removed
        """
        removed = 0
        
        for service_name in self.created_services[:]:
            try:
                # Stop service
                subprocess.run(
                    ["sc", "stop", service_name],
                    capture_output=True
                )
                
                # Delete service
                result = subprocess.run(
                    ["sc", "delete", service_name],
                    capture_output=True,
                    text=True
                )
                
                if result.returncode == 0:
                    removed += 1
                    
                self.created_services.remove(service_name)
                
            except Exception as e:
                logger.error(f"Failed to remove service {service_name}: {e}")
        
        return removed
    
    def clean_created_tasks(self) -> int:
        """
        Remove all scheduled tasks created during testing.
        
        Returns:
            Number of tasks removed
        """
        removed = 0
        
        for task_name in self.created_tasks[:]:
            try:
                result = subprocess.run(
                    ["schtasks", "/Delete", "/TN", task_name, "/F"],
                    capture_output=True,
                    text=True
                )
                
                if result.returncode == 0:
                    removed += 1
                    
                self.created_tasks.remove(task_name)
                
            except Exception as e:
                logger.error(f"Failed to remove task {task_name}: {e}")
        
        return removed
    
    def full_clean(self) -> Dict[str, int]:
        """
        Perform full system cleanup.
        
        Returns:
            Cleanup statistics
        """
        logger.info("Performing full system cleanup")
        
        results = {
            "registry_restored": self.restore_registry(),
            "files_restored": self.restore_files(),
            "files_removed": self.clean_created_files(),
            "services_removed": self.clean_created_services(),
            "tasks_removed": self.clean_created_tasks(),
        }
        
        logger.info(f"Cleanup complete: {results}")
        return results
    
    def emergency_clean(self):
        """
        Emergency cleanup - removes all known test artifacts aggressively.
        Use when normal cleanup fails.
        """
        logger.warning("Performing emergency cleanup")
        
        # Kill known malicious process names
        suspicious_processes = [
            "inject.exe", "payload.exe", "beacon.exe",
            "loader.exe", "dropper.exe"
        ]
        
        for proc_name in suspicious_processes:
            try:
                subprocess.run(
                    ["taskkill", "/IM", proc_name, "/F"],
                    capture_output=True
                )
            except:
                pass
        
        # Clean common artifact locations
        artifact_dirs = [
            os.environ.get("TEMP", ""),
            os.path.join(os.environ.get("APPDATA", ""), "Local", "Temp"),
        ]
        
        artifact_patterns = [
            "edr_test_*", "payload_*", "inject_*",
            "*.cpl", "*.hta", "malicious_*"
        ]
        
        for dir_path in artifact_dirs:
            if dir_path and Path(dir_path).exists():
                for pattern in artifact_patterns:
                    for file in Path(dir_path).glob(pattern):
                        try:
                            file.unlink()
                        except:
                            pass
        
        # Run normal cleanup
        self.full_clean()
