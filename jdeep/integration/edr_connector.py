"""
EDR Connector Module
====================

Provides connectors for various EDR/AV platforms.
Supports CrowdStrike, Microsoft Defender ATP, Carbon Black, and more.
"""

import logging
import subprocess
from typing import Dict, Any, List, Optional
from abc import ABC, abstractmethod

logger = logging.getLogger(__name__)


class BaseEDRConnector(ABC):
    """Abstract base class for EDR connectors."""
    
    @abstractmethod
    def connect(self, credentials: Dict[str, Any]) -> bool:
        """Connect to the EDR platform."""
        pass
    
    @abstractmethod
    def get_status(self) -> Dict[str, Any]:
        """Get EDR status."""
        pass
    
    @abstractmethod
    def get_alerts(self, time_range: int) -> List[Dict[str, Any]]:
        """Get alerts from the EDR."""
        pass


class CrowdStrikeConnector(BaseEDRConnector):
    """CrowdStrike Falcon connector."""
    
    def __init__(self):
        self.client_id: Optional[str] = None
        self.client_secret: Optional[str] = None
        self.base_url: str = "https://api.crowdstrike.com"
        self.token: Optional[str] = None
        self.connected: bool = False
    
    def connect(self, credentials: Dict[str, Any]) -> bool:
        """
        Connect to CrowdStrike Falcon.
        
        Args:
            credentials: Dict with client_id and client_secret
        """
        self.client_id = credentials.get("client_id")
        self.client_secret = credentials.get("client_secret")
        
        if not self.client_id or not self.client_secret:
            logger.error("CrowdStrike credentials missing")
            return False
        
        try:
            # Authenticate with OAuth2
            import requests
            
            response = requests.post(
                f"{self.base_url}/oauth2/token",
                data={
                    "client_id": self.client_id,
                    "client_secret": self.client_secret,
                },
                headers={"Content-Type": "application/x-www-form-urlencoded"}
            )
            
            if response.status_code == 201:
                self.token = response.json().get("access_token")
                self.connected = True
                logger.info("Connected to CrowdStrike Falcon")
                return True
            else:
                logger.error(f"CrowdStrike auth failed: {response.status_code}")
                return False
                
        except ImportError:
            logger.error("requests library not installed")
            return False
        except Exception as e:
            logger.error(f"CrowdStrike connection error: {e}")
            return False
    
    def get_status(self) -> Dict[str, Any]:
        """Get CrowdStrike sensor status."""
        return {
            "name": "CrowdStrike Falcon",
            "connected": self.connected,
            "running": self._check_sensor_running(),
        }
    
    def get_alerts(self, time_range: int = 3600) -> List[Dict[str, Any]]:
        """Get recent CrowdStrike alerts."""
        if not self.connected:
            return []
        
        try:
            import requests
            
            response = requests.get(
                f"{self.base_url}/alerts/queries/alerts/v1",
                headers={"Authorization": f"Bearer {self.token}"},
                params={"filter": f"created_timestamp:>now-{time_range}s"}
            )
            
            if response.status_code == 200:
                return response.json().get("resources", [])
            return []
            
        except Exception as e:
            logger.error(f"Error fetching CrowdStrike alerts: {e}")
            return []
    
    def _check_sensor_running(self) -> bool:
        """Check if CrowdStrike sensor is running."""
        try:
            result = subprocess.run(
                ["sc", "query", "csfalconservice"],
                capture_output=True,
                text=True
            )
            return "RUNNING" in result.stdout
        except:
            return False


class DefenderConnector(BaseEDRConnector):
    """Microsoft Defender ATP connector."""
    
    def __init__(self):
        self.tenant_id: Optional[str] = None
        self.client_id: Optional[str] = None
        self.client_secret: Optional[str] = None
        self.token: Optional[str] = None
        self.connected: bool = False
    
    def connect(self, credentials: Dict[str, Any]) -> bool:
        """
        Connect to Microsoft Defender ATP.
        
        Args:
            credentials: Dict with tenant_id, client_id, client_secret
        """
        self.tenant_id = credentials.get("tenant_id")
        self.client_id = credentials.get("client_id")
        self.client_secret = credentials.get("client_secret")
        
        if not all([self.tenant_id, self.client_id, self.client_secret]):
            logger.error("Defender ATP credentials missing")
            return False
        
        try:
            import requests
            
            response = requests.post(
                f"https://login.microsoftonline.com/{self.tenant_id}/oauth2/v2.0/token",
                data={
                    "client_id": self.client_id,
                    "client_secret": self.client_secret,
                    "scope": "https://api.securitycenter.microsoft.com/.default",
                    "grant_type": "client_credentials"
                }
            )
            
            if response.status_code == 200:
                self.token = response.json().get("access_token")
                self.connected = True
                logger.info("Connected to Microsoft Defender ATP")
                return True
            else:
                logger.error(f"Defender ATP auth failed: {response.status_code}")
                return False
                
        except Exception as e:
            logger.error(f"Defender ATP connection error: {e}")
            return False
    
    def get_status(self) -> Dict[str, Any]:
        """Get Defender status."""
        status = {
            "name": "Microsoft Defender ATP",
            "connected": self.connected,
            "running": False,
            "real_time_protection": False,
        }
        
        try:
            result = subprocess.run(
                ["powershell", "-Command", 
                 "Get-MpComputerStatus | Select-Object RealTimeProtectionEnabled, AntivirusEnabled"],
                capture_output=True,
                text=True
            )
            
            status["running"] = "True" in result.stdout
            status["real_time_protection"] = "RealTimeProtectionEnabled" in result.stdout and "True" in result.stdout
            
        except Exception as e:
            logger.debug(f"Error checking Defender status: {e}")
        
        return status
    
    def get_alerts(self, time_range: int = 3600) -> List[Dict[str, Any]]:
        """Get recent Defender alerts."""
        alerts = []
        
        try:
            # Get local Defender threat detections
            result = subprocess.run(
                ["powershell", "-Command",
                 f"Get-MpThreatDetection | Where-Object {{$_.InitialDetectionTime -gt (Get-Date).AddSeconds(-{time_range})}}"],
                capture_output=True,
                text=True
            )
            
            if result.stdout.strip():
                alerts.append({
                    "source": "Windows Defender",
                    "type": "ThreatDetection",
                    "details": result.stdout.strip()
                })
                
        except Exception as e:
            logger.debug(f"Error fetching Defender alerts: {e}")
        
        return alerts


class CarbonBlackConnector(BaseEDRConnector):
    """VMware Carbon Black connector."""
    
    def __init__(self):
        self.api_url: Optional[str] = None
        self.api_key: Optional[str] = None
        self.org_key: Optional[str] = None
        self.connected: bool = False
    
    def connect(self, credentials: Dict[str, Any]) -> bool:
        """Connect to Carbon Black."""
        self.api_url = credentials.get("api_url")
        self.api_key = credentials.get("api_key")
        self.org_key = credentials.get("org_key")
        
        if not all([self.api_url, self.api_key, self.org_key]):
            logger.error("Carbon Black credentials missing")
            return False
        
        # Validate connection
        try:
            import requests
            
            response = requests.get(
                f"{self.api_url}/appservices/v6/orgs/{self.org_key}/alerts",
                headers={
                    "X-Auth-Token": self.api_key,
                    "Content-Type": "application/json"
                }
            )
            
            self.connected = response.status_code == 200
            if self.connected:
                logger.info("Connected to Carbon Black")
            return self.connected
            
        except Exception as e:
            logger.error(f"Carbon Black connection error: {e}")
            return False
    
    def get_status(self) -> Dict[str, Any]:
        """Get Carbon Black status."""
        return {
            "name": "Carbon Black",
            "connected": self.connected,
            "running": self._check_sensor_running(),
        }
    
    def get_alerts(self, time_range: int = 3600) -> List[Dict[str, Any]]:
        """Get recent Carbon Black alerts."""
        if not self.connected:
            return []
        
        try:
            import requests
            
            response = requests.get(
                f"{self.api_url}/appservices/v6/orgs/{self.org_key}/alerts",
                headers={
                    "X-Auth-Token": self.api_key,
                    "Content-Type": "application/json"
                },
                params={"time_range": f"-{time_range}s"}
            )
            
            if response.status_code == 200:
                return response.json().get("results", [])
            return []
            
        except Exception as e:
            logger.error(f"Error fetching Carbon Black alerts: {e}")
            return []
    
    def _check_sensor_running(self) -> bool:
        """Check if Carbon Black sensor is running."""
        try:
            result = subprocess.run(
                ["sc", "query", "CbDefense"],
                capture_output=True,
                text=True
            )
            return "RUNNING" in result.stdout
        except:
            return False


class EDRConnector:
    """
    Main EDR connector class.
    
    Manages multiple EDR platform connections.
    """
    
    CONNECTORS = {
        "crowdstrike": CrowdStrikeConnector,
        "defender": DefenderConnector,
        "carbonblack": CarbonBlackConnector,
    }
    
    def __init__(self, config=None):
        """
        Initialize EDR Connector.
        
        Args:
            config: ConfigManager instance
        """
        self.config = config
        self.connectors: Dict[str, BaseEDRConnector] = {}
        
        logger.info("EDRConnector initialized")
    
    def connect(self, edr_name: str, credentials: Dict[str, Any]) -> bool:
        """
        Connect to an EDR platform.
        
        Args:
            edr_name: Name of EDR platform
            credentials: API credentials
            
        Returns:
            True if successful
        """
        edr_name = edr_name.lower()
        
        if edr_name not in self.CONNECTORS:
            logger.error(f"Unknown EDR: {edr_name}")
            return False
        
        connector = self.CONNECTORS[edr_name]()
        if connector.connect(credentials):
            self.connectors[edr_name] = connector
            return True
        
        return False
    
    def get_all_status(self) -> List[Dict[str, Any]]:
        """Get status of all EDR systems."""
        status_list = []
        
        # Check connected EDRs
        for name, connector in self.connectors.items():
            status_list.append(connector.get_status())
        
        # Check local EDR products even if not connected via API
        defender = DefenderConnector()
        defender_status = defender.get_status()
        if defender_status not in [s["name"] for s in status_list]:
            status_list.append(defender_status)
        
        return status_list
    
    def get_alerts(
        self,
        edr_name: Optional[str] = None,
        time_range: int = 3600
    ) -> List[Dict[str, Any]]:
        """
        Get alerts from EDR platforms.
        
        Args:
            edr_name: Specific EDR or None for all
            time_range: Time range in seconds
            
        Returns:
            List of alerts
        """
        alerts = []
        
        if edr_name:
            connector = self.connectors.get(edr_name.lower())
            if connector:
                alerts.extend(connector.get_alerts(time_range))
        else:
            for connector in self.connectors.values():
                alerts.extend(connector.get_alerts(time_range))
        
        return alerts
