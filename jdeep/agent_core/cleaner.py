"""
Artifact Cleaner Module
=======================

Handles cleanup of artifacts created during testing.
Removes files, registry entries, scheduled tasks, and other traces.
"""

import os
import logging
import winreg
import subprocess
from typing import Dict, Any, List, Optional
from pathlib import Path

logger = logging.getLogger(__name__)


class ArtifactCleaner:
    """
    Cleans up artifacts created during technique execution.
    
    Handles:
    - File artifacts (executables, DLLs, scripts)
    - Registry entries
    - Scheduled tasks
    - Windows services
    - Process termination
    """
    
    # Common artifact locations
    ARTIFACT_PATHS = [
        Path(os.environ.get("TEMP", "")),
        Path(os.environ.get("APPDATA", "")) / "Local" / "Temp",
        Path(os.environ.get("USERPROFILE", "")) / "AppData" / "Local" / "Temp",
        Path("C:/Windows/Temp"),
    ]
    
    # Registry keys commonly modified by malware
    REGISTRY_KEYS = [
        (winreg.HKEY_CURRENT_USER, r"Software\Microsoft\Windows\CurrentVersion\Run"),
        (winreg.HKEY_CURRENT_USER, r"Software\Microsoft\Windows\CurrentVersion\RunOnce"),
        (winreg.HKEY_LOCAL_MACHINE, r"Software\Microsoft\Windows\CurrentVersion\Run"),
        (winreg.HKEY_LOCAL_MACHINE, r"Software\Microsoft\Windows\CurrentVersion\RunOnce"),
    ]
    
    def __init__(self, config=None):
        """
        Initialize the Artifact Cleaner.
        
        Args:
            config: ConfigManager instance
        """
        self.config = config
        self.tracked_artifacts: List[str] = []
        self.tracked_registry: List[tuple] = []
        self.tracked_tasks: List[str] = []
        self.tracked_services: List[str] = []
        self.tracked_processes: List[int] = []
        
        logger.info("ArtifactCleaner initialized")
    
    def track_artifact(self, path: str):
        """Track a file artifact for later cleanup."""
        self.tracked_artifacts.append(path)
        logger.debug(f"Tracking artifact: {path}")
    
    def track_registry(self, hive: int, key: str, value: str):
        """Track a registry entry for later cleanup."""
        self.tracked_registry.append((hive, key, value))
        logger.debug(f"Tracking registry: {key}\\{value}")
    
    def track_task(self, task_name: str):
        """Track a scheduled task for later cleanup."""
        self.tracked_tasks.append(task_name)
        logger.debug(f"Tracking task: {task_name}")
    
    def track_service(self, service_name: str):
        """Track a Windows service for later cleanup."""
        self.tracked_services.append(service_name)
        logger.debug(f"Tracking service: {service_name}")
    
    def track_process(self, pid: int):
        """Track a process for later termination."""
        self.tracked_processes.append(pid)
        logger.debug(f"Tracking process: {pid}")
    
    def clean(
        self,
        target: str = "localhost",
        clean_artifacts: bool = True,
        clean_registry: bool = True,
        kill_processes: bool = False
    ) -> Dict[str, Any]:
        """
        Perform cleanup operations.
        
        Args:
            target: Target system (localhost for local)
            clean_artifacts: Remove file artifacts
            clean_registry: Clean registry entries
            kill_processes: Terminate tracked processes
            
        Returns:
            Cleanup results dictionary
        """
        results = {
            "files_removed": 0,
            "registry_cleaned": 0,
            "tasks_removed": 0,
            "services_removed": 0,
            "processes_killed": 0,
            "errors": []
        }
        
        if clean_artifacts:
            results["files_removed"] = self._clean_files()
        
        if clean_registry:
            results["registry_cleaned"] = self._clean_registry()
        
        if kill_processes:
            results["processes_killed"] = self._kill_processes()
        
        # Clean tracked tasks and services
        results["tasks_removed"] = self._clean_tasks()
        results["services_removed"] = self._clean_services()
        
        logger.info(f"Cleanup complete: {results}")
        return results
    
    def clean_artifacts(self, artifacts: List[str]) -> int:
        """
        Clean specific artifacts.
        
        Args:
            artifacts: List of file paths to remove
            
        Returns:
            Number of artifacts removed
        """
        removed = 0
        for artifact in artifacts:
            try:
                path = Path(artifact)
                if path.exists():
                    if path.is_file():
                        path.unlink()
                    elif path.is_dir():
                        import shutil
                        shutil.rmtree(path)
                    removed += 1
                    logger.debug(f"Removed: {artifact}")
            except Exception as e:
                logger.warning(f"Failed to remove {artifact}: {e}")
        
        return removed
    
    def clean_technique_artifacts(self, technique_id: str):
        """
        Clean artifacts specific to a technique.
        
        Args:
            technique_id: MITRE ATT&CK technique ID
        """
        # Technique-specific cleanup patterns
        patterns = {
            "T1055": ["*.dll", "injected_*"],
            "T1218.011": ["*.cpl"],
            "T1218.005": ["*.hta"],
            "T1059.001": ["*.ps1", "encoded_*"],
        }
        
        cleanup_patterns = patterns.get(technique_id, [])
        
        for base_path in self.ARTIFACT_PATHS:
            if base_path.exists():
                for pattern in cleanup_patterns:
                    for file in base_path.glob(pattern):
                        try:
                            file.unlink()
                            logger.debug(f"Removed technique artifact: {file}")
                        except Exception as e:
                            logger.warning(f"Failed to remove {file}: {e}")
    
    def deep_clean(self):
        """
        Perform deep cleanup - removes all known test artifacts.
        Use with caution.
        """
        logger.warning("Performing deep clean - this will remove all test artifacts")
        
        # Clean all tracked items
        self._clean_files()
        self._clean_registry()
        self._clean_tasks()
        self._clean_services()
        self._kill_processes()
        
        # Clean common artifact locations
        artifact_patterns = [
            "edr_test_*",
            "payload_*",
            "inject_*",
            "*.cpl",
            "*.hta",
            "malicious_*",
        ]
        
        for base_path in self.ARTIFACT_PATHS:
            if base_path.exists():
                for pattern in artifact_patterns:
                    for file in base_path.glob(pattern):
                        try:
                            file.unlink()
                            logger.debug(f"Deep clean removed: {file}")
                        except:
                            pass
    
    def _clean_files(self) -> int:
        """Clean tracked file artifacts."""
        removed = 0
        for artifact in self.tracked_artifacts[:]:
            try:
                path = Path(artifact)
                if path.exists():
                    path.unlink()
                    removed += 1
                self.tracked_artifacts.remove(artifact)
            except Exception as e:
                logger.warning(f"Failed to remove file {artifact}: {e}")
        
        return removed
    
    def _clean_registry(self) -> int:
        """Clean tracked registry entries."""
        cleaned = 0
        for hive, key, value in self.tracked_registry[:]:
            try:
                with winreg.OpenKey(hive, key, 0, winreg.KEY_SET_VALUE) as reg_key:
                    winreg.DeleteValue(reg_key, value)
                    cleaned += 1
                self.tracked_registry.remove((hive, key, value))
            except FileNotFoundError:
                # Value doesn't exist
                self.tracked_registry.remove((hive, key, value))
            except Exception as e:
                logger.warning(f"Failed to clean registry {key}\\{value}: {e}")
        
        return cleaned
    
    def _clean_tasks(self) -> int:
        """Clean tracked scheduled tasks."""
        removed = 0
        for task_name in self.tracked_tasks[:]:
            try:
                result = subprocess.run(
                    ["schtasks", "/Delete", "/TN", task_name, "/F"],
                    capture_output=True,
                    text=True
                )
                if result.returncode == 0:
                    removed += 1
                self.tracked_tasks.remove(task_name)
            except Exception as e:
                logger.warning(f"Failed to remove task {task_name}: {e}")
        
        return removed
    
    def _clean_services(self) -> int:
        """Clean tracked Windows services."""
        removed = 0
        for service_name in self.tracked_services[:]:
            try:
                # Stop the service first
                subprocess.run(
                    ["sc", "stop", service_name],
                    capture_output=True
                )
                # Delete the service
                result = subprocess.run(
                    ["sc", "delete", service_name],
                    capture_output=True,
                    text=True
                )
                if result.returncode == 0:
                    removed += 1
                self.tracked_services.remove(service_name)
            except Exception as e:
                logger.warning(f"Failed to remove service {service_name}: {e}")
        
        return removed
    
    def _kill_processes(self) -> int:
        """Terminate tracked processes."""
        killed = 0
        for pid in self.tracked_processes[:]:
            try:
                result = subprocess.run(
                    ["taskkill", "/PID", str(pid), "/F"],
                    capture_output=True,
                    text=True
                )
                if result.returncode == 0:
                    killed += 1
                self.tracked_processes.remove(pid)
            except Exception as e:
                logger.warning(f"Failed to kill process {pid}: {e}")
        
        return killed
