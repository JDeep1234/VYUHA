"""
Telemetry Monitor Module
========================

Monitors EDR/AV responses and captures security events.
Tracks detections, alerts, and system events during technique execution.
"""

import logging
import time
import threading
from typing import Dict, Any, List, Optional, Callable
from datetime import datetime
from collections import deque

logger = logging.getLogger(__name__)


class TelemetryMonitor:
    """
    Monitors EDR responses and system telemetry during technique execution.
    
    Captures:
    - EDR alerts and detections
    - Windows Event Logs
    - Process creation events
    - File system events
    - Registry modifications
    - Network connections
    """
    
    def __init__(self, config=None):
        """
        Initialize the Telemetry Monitor.
        
        Args:
            config: ConfigManager instance
        """
        self.config = config
        self.is_monitoring = False
        self.monitor_thread: Optional[threading.Thread] = None
        
        # Event buffers
        self.events: deque = deque(maxlen=1000)
        self.alerts: List[Dict[str, Any]] = []
        self.detections: List[Dict[str, Any]] = []
        
        # Monitoring start time
        self.start_time: Optional[datetime] = None
        
        # Event callbacks
        self.callbacks: Dict[str, List[Callable]] = {
            "on_alert": [],
            "on_detection": [],
            "on_event": [],
        }
        
        logger.info("TelemetryMonitor initialized")
    
    def add_callback(self, event_type: str, callback: Callable):
        """
        Add callback for specific event types.
        
        Args:
            event_type: Type of event (on_alert, on_detection, on_event)
            callback: Callback function
        """
        if event_type in self.callbacks:
            self.callbacks[event_type].append(callback)
    
    def start_monitoring(self):
        """Start telemetry monitoring."""
        if self.is_monitoring:
            logger.warning("Monitoring already active")
            return
        
        self.is_monitoring = True
        self.start_time = datetime.now()
        self.events.clear()
        self.alerts.clear()
        self.detections.clear()
        
        # Start background monitoring thread
        self.monitor_thread = threading.Thread(target=self._monitor_loop, daemon=True)
        self.monitor_thread.start()
        
        logger.info("Telemetry monitoring started")
    
    def stop_monitoring(self) -> Dict[str, Any]:
        """
        Stop monitoring and return captured telemetry.
        
        Returns:
            Telemetry data dictionary
        """
        self.is_monitoring = False
        
        if self.monitor_thread:
            self.monitor_thread.join(timeout=2)
        
        end_time = datetime.now()
        duration = (end_time - self.start_time).total_seconds() if self.start_time else 0
        
        telemetry = {
            "start_time": self.start_time.isoformat() if self.start_time else None,
            "end_time": end_time.isoformat(),
            "duration": duration,
            "detected": len(self.detections) > 0,
            "alerts": list(self.alerts),
            "detections": list(self.detections),
            "events": list(self.events),
            "summary": self._generate_summary(),
        }
        
        logger.info(f"Telemetry monitoring stopped - Captured {len(self.events)} events")
        
        return telemetry
    
    def _monitor_loop(self):
        """Background monitoring loop."""
        while self.is_monitoring:
            try:
                # Monitor Windows Event Logs
                self._check_security_events()
                
                # Monitor for EDR alerts
                self._check_edr_alerts()
                
                # Monitor process events
                self._check_process_events()
                
                time.sleep(0.5)  # Poll interval
                
            except Exception as e:
                logger.error(f"Monitoring error: {e}")
    
    def _check_security_events(self):
        """Check Windows Security Event Log for relevant events."""
        try:
            import win32evtlog
            import win32evtlogutil
            
            # Relevant security event IDs
            relevant_events = {
                4688: "Process Created",
                4689: "Process Terminated",
                4657: "Registry Value Modified",
                4663: "Object Access Attempt",
                4698: "Scheduled Task Created",
                4699: "Scheduled Task Deleted",
                4697: "Service Installed",
                5140: "Network Share Accessed",
                5145: "Network Share Object Checked",
            }
            
            hand = win32evtlog.OpenEventLog(None, "Security")
            flags = win32evtlog.EVENTLOG_BACKWARDS_READ | win32evtlog.EVENTLOG_SEQUENTIAL_READ
            
            events = win32evtlog.ReadEventLog(hand, flags, 0)
            
            for event in events:
                event_id = event.EventID & 0xFFFF
                if event_id in relevant_events:
                    event_time = event.TimeGenerated
                    
                    # Only process events after monitoring started
                    if self.start_time and event_time < self.start_time:
                        break
                    
                    event_data = {
                        "event_id": event_id,
                        "description": relevant_events[event_id],
                        "time": str(event_time),
                        "source": event.SourceName,
                        "category": event.EventCategory,
                    }
                    
                    self.events.append(event_data)
                    self._trigger_callbacks("on_event", event_data)
            
            win32evtlog.CloseEventLog(hand)
            
        except ImportError:
            # win32evtlog not available
            pass
        except Exception as e:
            logger.debug(f"Error checking security events: {e}")
    
    def _check_edr_alerts(self):
        """Check for EDR-specific alerts."""
        # This would integrate with specific EDR APIs
        # For now, we simulate by checking for common indicators
        
        try:
            # Check Windows Defender alerts via WMI
            import subprocess
            result = subprocess.run(
                ["powershell", "-Command",
                 "Get-MpThreatDetection | Where-Object {$_.InitialDetectionTime -gt (Get-Date).AddMinutes(-5)} | Select-Object -First 5"],
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if result.stdout.strip():
                alert = {
                    "source": "Windows Defender",
                    "type": "ThreatDetection",
                    "time": datetime.now().isoformat(),
                    "details": result.stdout.strip()
                }
                self.alerts.append(alert)
                self.detections.append(alert)
                self._trigger_callbacks("on_alert", alert)
                self._trigger_callbacks("on_detection", alert)
                
        except Exception as e:
            logger.debug(f"Error checking EDR alerts: {e}")
    
    def _check_process_events(self):
        """Check for suspicious process events."""
        try:
            import psutil
            
            # Get current processes
            for proc in psutil.process_iter(['pid', 'name', 'cmdline', 'create_time']):
                try:
                    create_time = datetime.fromtimestamp(proc.info['create_time'])
                    
                    if self.start_time and create_time > self.start_time:
                        # New process created during monitoring
                        event = {
                            "type": "process_created",
                            "pid": proc.info['pid'],
                            "name": proc.info['name'],
                            "cmdline": proc.info['cmdline'],
                            "time": create_time.isoformat()
                        }
                        self.events.append(event)
                        
                except (psutil.NoSuchProcess, psutil.AccessDenied):
                    pass
                    
        except ImportError:
            pass
        except Exception as e:
            logger.debug(f"Error checking process events: {e}")
    
    def _trigger_callbacks(self, event_type: str, data: Dict[str, Any]):
        """Trigger callbacks for event type."""
        for callback in self.callbacks.get(event_type, []):
            try:
                callback(data)
            except Exception as e:
                logger.error(f"Callback error: {e}")
    
    def _generate_summary(self) -> Dict[str, Any]:
        """Generate summary of captured telemetry."""
        event_types = {}
        for event in self.events:
            event_type = event.get("type") or event.get("description", "unknown")
            event_types[event_type] = event_types.get(event_type, 0) + 1
        
        return {
            "total_events": len(self.events),
            "total_alerts": len(self.alerts),
            "total_detections": len(self.detections),
            "event_breakdown": event_types,
            "process_created": any(e.get("type") == "process_created" for e in self.events),
            "file_written": any(e.get("event_id") == 4663 for e in self.events),
            "registry_modified": any(e.get("event_id") == 4657 for e in self.events),
        }
    
    def get_edr_status(self) -> List[Dict[str, Any]]:
        """
        Get status of detected EDR/AV products.
        
        Returns:
            List of EDR/AV status dictionaries
        """
        edr_list = []
        
        # Check Windows Defender
        try:
            import subprocess
            result = subprocess.run(
                ["powershell", "-Command", 
                 "Get-MpComputerStatus | Select-Object AMRunningMode, AntivirusEnabled, RealTimeProtectionEnabled"],
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if "True" in result.stdout:
                edr_list.append({
                    "name": "Windows Defender",
                    "running": True,
                    "version": "Built-in"
                })
        except:
            pass
        
        # Check for other EDR products by process name
        edr_processes = {
            "csfalconservice": "CrowdStrike Falcon",
            "cb.exe": "Carbon Black",
            "SentinelAgent": "SentinelOne",
            "sepmaster": "Symantec Endpoint Protection",
            "ekrn": "ESET",
            "avp": "Kaspersky",
            "sophosfs": "Sophos",
        }
        
        try:
            import psutil
            running_processes = {p.name().lower() for p in psutil.process_iter(['name'])}
            
            for proc_name, edr_name in edr_processes.items():
                if proc_name.lower() in running_processes:
                    edr_list.append({
                        "name": edr_name,
                        "running": True,
                        "version": "Detected"
                    })
        except:
            pass
        
        return edr_list
