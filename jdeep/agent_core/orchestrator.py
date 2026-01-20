"""
Orchestrator Module
===================

Manages execution flow and state machine for technique execution.
Coordinates between different attack modules and handles execution lifecycle.
"""

import logging
import time
from typing import Dict, Any, Optional, Callable
from enum import Enum, auto
from dataclasses import dataclass, field
from pathlib import Path

logger = logging.getLogger(__name__)


class ExecutionState(Enum):
    """Execution state machine states."""
    IDLE = auto()
    PREPARING = auto()
    EXECUTING = auto()
    MONITORING = auto()
    CLEANING = auto()
    COMPLETED = auto()
    FAILED = auto()


@dataclass
class ExecutionContext:
    """Context for technique execution."""
    technique_id: str
    target: str
    options: Dict[str, Any] = field(default_factory=dict)
    state: ExecutionState = ExecutionState.IDLE
    start_time: float = 0.0
    artifacts: list = field(default_factory=list)
    errors: list = field(default_factory=list)


class Orchestrator:
    """
    Orchestrates the execution of attack techniques.
    
    Manages the execution flow:
    1. Preparation (load technique, validate target)
    2. Execution (run the technique)
    3. Monitoring (capture telemetry)
    4. Cleanup (remove artifacts if needed)
    """
    
    def __init__(self, config=None):
        """
        Initialize the Orchestrator.
        
        Args:
            config: ConfigManager instance
        """
        self.config = config
        self.current_context: Optional[ExecutionContext] = None
        self.technique_registry: Dict[str, Callable] = {}
        self.hooks: Dict[str, list] = {
            "pre_execute": [],
            "post_execute": [],
            "on_error": [],
        }
        
        # Load built-in techniques
        self._register_builtin_techniques()
        
        logger.info("Orchestrator initialized")
    
    def _register_builtin_techniques(self):
        """Register built-in technique handlers."""
        # These will be replaced with actual technique implementations
        # from the exploit scripts module (Bipin's work)
        
        self.technique_registry = {
            # Defense Evasion
            "T1055": self._technique_process_injection,
            "T1055.001": self._technique_dll_injection,
            "T1055.012": self._technique_process_hollowing,
            "T1218.001": self._technique_compiled_html,
            "T1218.005": self._technique_mshta,
            "T1218.011": self._technique_rundll32,
            "T1574.002": self._technique_dll_sideloading,
            
            # Execution
            "T1059.001": self._technique_powershell,
            "T1047": self._technique_wmi,
            
            # Credential Access
            "T1003.001": self._technique_lsass_dump,
            
            # Persistence
            "T1053.005": self._technique_scheduled_task,
            "T1543.003": self._technique_windows_service,
        }
    
    def register_technique(self, technique_id: str, handler: Callable):
        """
        Register a custom technique handler.
        
        Args:
            technique_id: MITRE ATT&CK technique ID
            handler: Callable that executes the technique
        """
        self.technique_registry[technique_id] = handler
        logger.debug(f"Registered technique: {technique_id}")
    
    def add_hook(self, event: str, callback: Callable):
        """
        Add execution hook.
        
        Args:
            event: Hook event (pre_execute, post_execute, on_error)
            callback: Callback function
        """
        if event in self.hooks:
            self.hooks[event].append(callback)
    
    def execute(
        self,
        technique_id: str,
        target: str,
        options: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """
        Execute a technique.
        
        Args:
            technique_id: MITRE ATT&CK technique ID
            target: Target system
            options: Execution options
            
        Returns:
            Execution result dictionary
        """
        options = options or {}
        
        # Create execution context
        self.current_context = ExecutionContext(
            technique_id=technique_id,
            target=target,
            options=options,
            start_time=time.time()
        )
        
        result = {
            "technique": technique_id,
            "target": target,
            "success": False,
            "artifacts": [],
            "output": None,
            "error": None,
        }
        
        try:
            # Preparation phase
            self._transition_state(ExecutionState.PREPARING)
            self._prepare_execution()
            
            # Run pre-execution hooks
            for hook in self.hooks["pre_execute"]:
                hook(self.current_context)
            
            # Execution phase
            self._transition_state(ExecutionState.EXECUTING)
            
            # Get technique handler
            handler = self.technique_registry.get(technique_id)
            if not handler:
                raise ValueError(f"Unknown technique: {technique_id}")
            
            # Execute technique
            exec_result = handler(target, options)
            
            result["success"] = exec_result.get("success", True)
            result["output"] = exec_result.get("output")
            result["artifacts"] = exec_result.get("artifacts", [])
            
            # Store artifacts in context
            self.current_context.artifacts.extend(result["artifacts"])
            
            # Monitoring phase
            self._transition_state(ExecutionState.MONITORING)
            time.sleep(options.get("monitor_delay", 1))  # Allow EDR to process
            
            # Run post-execution hooks
            for hook in self.hooks["post_execute"]:
                hook(self.current_context, result)
            
            # Cleanup phase (optional)
            if options.get("auto_cleanup", False):
                self._transition_state(ExecutionState.CLEANING)
                self._cleanup_artifacts()
            
            self._transition_state(ExecutionState.COMPLETED)
            
        except Exception as e:
            logger.error(f"Execution failed: {e}")
            self._transition_state(ExecutionState.FAILED)
            result["error"] = str(e)
            self.current_context.errors.append(str(e))
            
            # Run error hooks
            for hook in self.hooks["on_error"]:
                hook(self.current_context, e)
        
        return result
    
    def _transition_state(self, new_state: ExecutionState):
        """Transition to a new execution state."""
        if self.current_context:
            old_state = self.current_context.state
            self.current_context.state = new_state
            logger.debug(f"State transition: {old_state.name} -> {new_state.name}")
    
    def _prepare_execution(self):
        """Prepare for technique execution."""
        # Validate target
        # Load any required resources
        # Check prerequisites
        pass
    
    def _cleanup_artifacts(self):
        """Clean up artifacts created during execution."""
        if self.current_context:
            for artifact in self.current_context.artifacts:
                try:
                    path = Path(artifact)
                    if path.exists():
                        path.unlink()
                        logger.debug(f"Removed artifact: {artifact}")
                except Exception as e:
                    logger.warning(f"Failed to remove artifact {artifact}: {e}")
    
    # Technique Handlers (Stubs - to be implemented by exploit scripts module)
    
    def _technique_process_injection(self, target: str, options: Dict) -> Dict:
        """T1055 - Process Injection."""
        logger.info(f"Executing Process Injection on {target}")
        # Placeholder - actual implementation in exploit scripts module
        return {"success": True, "output": "Process injection simulated", "artifacts": []}
    
    def _technique_dll_injection(self, target: str, options: Dict) -> Dict:
        """T1055.001 - DLL Injection."""
        logger.info(f"Executing DLL Injection on {target}")
        return {"success": True, "output": "DLL injection simulated", "artifacts": []}
    
    def _technique_process_hollowing(self, target: str, options: Dict) -> Dict:
        """T1055.012 - Process Hollowing."""
        logger.info(f"Executing Process Hollowing on {target}")
        return {"success": True, "output": "Process hollowing simulated", "artifacts": []}
    
    def _technique_compiled_html(self, target: str, options: Dict) -> Dict:
        """T1218.001 - Compiled HTML File."""
        logger.info(f"Executing CHM attack on {target}")
        return {"success": True, "output": "CHM execution simulated", "artifacts": []}
    
    def _technique_mshta(self, target: str, options: Dict) -> Dict:
        """T1218.005 - Mshta."""
        logger.info(f"Executing Mshta attack on {target}")
        return {"success": True, "output": "Mshta execution simulated", "artifacts": []}
    
    def _technique_rundll32(self, target: str, options: Dict) -> Dict:
        """T1218.011 - Rundll32 (CPL Attack)."""
        logger.info(f"Executing Rundll32/CPL attack on {target}")
        return {"success": True, "output": "Rundll32 execution simulated", "artifacts": []}
    
    def _technique_dll_sideloading(self, target: str, options: Dict) -> Dict:
        """T1574.002 - DLL Side-Loading."""
        logger.info(f"Executing DLL Sideloading on {target}")
        return {"success": True, "output": "DLL sideloading simulated", "artifacts": []}
    
    def _technique_powershell(self, target: str, options: Dict) -> Dict:
        """T1059.001 - PowerShell."""
        logger.info(f"Executing PowerShell technique on {target}")
        return {"success": True, "output": "PowerShell execution simulated", "artifacts": []}
    
    def _technique_wmi(self, target: str, options: Dict) -> Dict:
        """T1047 - Windows Management Instrumentation."""
        logger.info(f"Executing WMI technique on {target}")
        return {"success": True, "output": "WMI execution simulated", "artifacts": []}
    
    def _technique_lsass_dump(self, target: str, options: Dict) -> Dict:
        """T1003.001 - LSASS Memory."""
        logger.info(f"Executing LSASS dump on {target}")
        return {"success": True, "output": "LSASS dump simulated", "artifacts": []}
    
    def _technique_scheduled_task(self, target: str, options: Dict) -> Dict:
        """T1053.005 - Scheduled Task."""
        logger.info(f"Creating scheduled task on {target}")
        return {"success": True, "output": "Scheduled task created", "artifacts": []}
    
    def _technique_windows_service(self, target: str, options: Dict) -> Dict:
        """T1543.003 - Windows Service."""
        logger.info(f"Creating Windows service on {target}")
        return {"success": True, "output": "Windows service created", "artifacts": []}
