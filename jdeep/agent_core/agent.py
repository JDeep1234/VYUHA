"""
Agent Core - Main Agent Class
=============================

Central agent that coordinates all core components.
"""

import time
import logging
from typing import Dict, Any, List, Optional
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed

from jdeep.agent_core.orchestrator import Orchestrator
from jdeep.agent_core.output_handler import OutputHandler
from jdeep.agent_core.cleaner import ArtifactCleaner
from jdeep.agent_core.telemetry import TelemetryMonitor
from jdeep.config import ConfigManager

logger = logging.getLogger(__name__)


class AgentCore:
    """
    Main agent class that coordinates execution of techniques
    and manages the testing lifecycle.
    """
    
    def __init__(self, config_path: Optional[str] = None, verbose: bool = False):
        """
        Initialize the Agent Core.
        
        Args:
            config_path: Path to configuration file
            verbose: Enable verbose logging
        """
        self.verbose = verbose
        self._setup_logging()
        
        # Load configuration
        self.config = ConfigManager(config_path)
        
        # Initialize components
        self.orchestrator = Orchestrator(self.config)
        self.output_handler = OutputHandler(self.config)
        self.cleaner = ArtifactCleaner(self.config)
        self.telemetry = TelemetryMonitor(self.config)
        
        # State tracking
        self.session_id = self._generate_session_id()
        self.results: List[Dict[str, Any]] = []
        self.artifacts: List[str] = []
        
        logger.info(f"AgentCore initialized - Session: {self.session_id}")
    
    def _setup_logging(self):
        """Configure logging based on verbosity."""
        level = logging.DEBUG if self.verbose else logging.INFO
        logging.basicConfig(
            level=level,
            format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
        )
    
    def _generate_session_id(self) -> str:
        """Generate unique session identifier."""
        import uuid
        return str(uuid.uuid4())[:8]
    
    def execute_technique(
        self,
        technique_id: str,
        target: str,
        options: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """
        Execute a single ATT&CK technique.
        
        Args:
            technique_id: MITRE ATT&CK technique ID (e.g., T1055)
            target: Target system
            options: Additional execution options
            
        Returns:
            Execution result dictionary
        """
        logger.info(f"Executing technique {technique_id} on {target}")
        
        start_time = time.time()
        result = {
            "technique": technique_id,
            "target": target,
            "session_id": self.session_id,
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "success": False,
            "detected": False,
            "evasion_score": 0.0,
            "duration": 0.0,
            "alerts": [],
            "artifacts": [],
            "telemetry": {},
        }
        
        try:
            # Start telemetry monitoring
            self.telemetry.start_monitoring()
            
            # Execute through orchestrator
            exec_result = self.orchestrator.execute(technique_id, target, options)
            
            # Update result
            result.update(exec_result)
            result["success"] = exec_result.get("success", False)
            
            # Capture telemetry
            telemetry_data = self.telemetry.stop_monitoring()
            result["telemetry"] = telemetry_data
            result["detected"] = telemetry_data.get("detected", False)
            result["alerts"] = telemetry_data.get("alerts", [])
            
            # Calculate evasion score
            result["evasion_score"] = self._calculate_evasion_score(result)
            
            # Track artifacts
            result["artifacts"] = exec_result.get("artifacts", [])
            self.artifacts.extend(result["artifacts"])
            
        except Exception as e:
            logger.error(f"Technique execution failed: {e}")
            result["error"] = str(e)
            
        finally:
            result["duration"] = time.time() - start_time
            self.results.append(result)
            
            # Output result
            self.output_handler.log_result(result)
        
        return result
    
    def execute_campaign(
        self,
        campaign_config: Dict[str, Any],
        target: str,
        parallel: bool = False
    ) -> Dict[str, Any]:
        """
        Execute a full attack campaign.
        
        Args:
            campaign_config: Campaign configuration dictionary
            target: Target system
            parallel: Execute techniques in parallel
            
        Returns:
            Campaign results dictionary
        """
        campaign_name = campaign_config.get("name", "Unknown Campaign")
        techniques = campaign_config.get("techniques", [])
        
        logger.info(f"Starting campaign: {campaign_name} with {len(techniques)} techniques")
        
        campaign_result = {
            "campaign": campaign_name,
            "session_id": self.session_id,
            "target": target,
            "start_time": time.strftime("%Y-%m-%d %H:%M:%S"),
            "techniques_total": len(techniques),
            "techniques_success": 0,
            "techniques_detected": 0,
            "results": [],
            "average_evasion_score": 0.0,
        }
        
        if parallel:
            results = self._execute_parallel(techniques, target)
        else:
            results = self._execute_sequential(techniques, target)
        
        campaign_result["results"] = results
        campaign_result["techniques_success"] = sum(1 for r in results if r.get("success"))
        campaign_result["techniques_detected"] = sum(1 for r in results if r.get("detected"))
        
        if results:
            campaign_result["average_evasion_score"] = sum(
                r.get("evasion_score", 0) for r in results
            ) / len(results)
        
        campaign_result["end_time"] = time.strftime("%Y-%m-%d %H:%M:%S")
        
        return campaign_result
    
    def _execute_sequential(
        self,
        techniques: List[Dict[str, Any]],
        target: str
    ) -> List[Dict[str, Any]]:
        """Execute techniques sequentially."""
        results = []
        for tech in techniques:
            technique_id = tech.get("id") or tech.get("technique")
            options = tech.get("options", {})
            
            # Pre-execution delay if specified
            if delay := tech.get("delay"):
                time.sleep(delay)
            
            result = self.execute_technique(technique_id, target, options)
            results.append(result)
            
            # Post-execution cleanup if specified
            if tech.get("cleanup", True):
                self.cleaner.clean_technique_artifacts(technique_id)
        
        return results
    
    def _execute_parallel(
        self,
        techniques: List[Dict[str, Any]],
        target: str,
        max_workers: int = 4
    ) -> List[Dict[str, Any]]:
        """Execute techniques in parallel."""
        results = []
        
        with ThreadPoolExecutor(max_workers=max_workers) as executor:
            futures = {}
            for tech in techniques:
                technique_id = tech.get("id") or tech.get("technique")
                options = tech.get("options", {})
                future = executor.submit(
                    self.execute_technique, technique_id, target, options
                )
                futures[future] = technique_id
            
            for future in as_completed(futures):
                try:
                    result = future.result()
                    results.append(result)
                except Exception as e:
                    logger.error(f"Parallel execution error: {e}")
                    results.append({
                        "technique": futures[future],
                        "success": False,
                        "error": str(e)
                    })
        
        return results
    
    def _calculate_evasion_score(self, result: Dict[str, Any]) -> float:
        """
        Calculate evasion score based on detection and telemetry.
        
        Score ranges from 0.0 (fully detected) to 1.0 (fully evaded).
        """
        score = 1.0
        
        # Detection penalty
        if result.get("detected"):
            score -= 0.5
        
        # Alert penalties
        alerts = result.get("alerts", [])
        score -= len(alerts) * 0.1
        
        # Telemetry penalties
        telemetry = result.get("telemetry", {})
        if telemetry.get("process_created"):
            score -= 0.1
        if telemetry.get("file_written"):
            score -= 0.1
        if telemetry.get("registry_modified"):
            score -= 0.1
        
        return max(0.0, min(1.0, score))
    
    def cleanup(self, full_cleanup: bool = False):
        """
        Clean up all artifacts from the session.
        
        Args:
            full_cleanup: Perform deep cleanup including registry
        """
        logger.info(f"Cleaning up session {self.session_id}")
        
        self.cleaner.clean_artifacts(self.artifacts)
        
        if full_cleanup:
            self.cleaner.deep_clean()
        
        self.artifacts.clear()
    
    def get_session_summary(self) -> Dict[str, Any]:
        """Get summary of current session."""
        return {
            "session_id": self.session_id,
            "total_techniques": len(self.results),
            "successful": sum(1 for r in self.results if r.get("success")),
            "detected": sum(1 for r in self.results if r.get("detected")),
            "average_evasion": sum(r.get("evasion_score", 0) for r in self.results) / len(self.results) if self.results else 0,
            "artifacts_created": len(self.artifacts),
        }
