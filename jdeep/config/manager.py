"""
Configuration Manager
=====================

Centralized configuration management for the EDR Framework.
Supports YAML, JSON files and environment variables.
"""

import os
import logging
from typing import Any, Dict, Optional
from pathlib import Path

logger = logging.getLogger(__name__)


class ConfigManager:
    """
    Configuration manager for the EDR Framework.
    
    Supports:
    - YAML configuration files
    - JSON configuration files
    - Environment variables
    - Default values
    """
    
    # Default configuration
    DEFAULTS = {
        # General
        "debug": False,
        "log_level": "INFO",
        "output_dir": "results",
        
        # Agent settings
        "agent": {
            "session_timeout": 3600,
            "auto_cleanup": True,
            "monitor_delay": 1,
        },
        
        # Telemetry settings
        "telemetry": {
            "enabled": True,
            "poll_interval": 0.5,
            "max_events": 1000,
        },
        
        # EDR integration
        "edr": {
            "auto_detect": True,
            "providers": ["defender", "crowdstrike", "carbonblack"],
        },
        
        # Snapshot settings
        "snapshot": {
            "provider": "auto",  # auto, hyperv, virtualbox, vmware
            "auto_restore": False,
        },
        
        # Report settings
        "reports": {
            "format": "html",
            "include_telemetry": True,
            "include_mitre_mapping": True,
        },
        
        # Security settings
        "security": {
            "encrypt_payloads": True,
            "audit_logging": True,
        },
    }
    
    def __init__(self, config_path: Optional[str] = None):
        """
        Initialize Configuration Manager.
        
        Args:
            config_path: Path to configuration file
        """
        self.config: Dict[str, Any] = self.DEFAULTS.copy()
        self.config_path = config_path
        
        # Load configuration
        if config_path:
            self._load_file(config_path)
        else:
            self._load_default_paths()
        
        # Override with environment variables
        self._load_env_vars()
        
        logger.info("ConfigManager initialized")
    
    def _load_file(self, config_path: str):
        """Load configuration from file."""
        path = Path(config_path)
        
        if not path.exists():
            logger.warning(f"Config file not found: {config_path}")
            return
        
        try:
            if path.suffix in ['.yaml', '.yml']:
                import yaml
                with open(path) as f:
                    file_config = yaml.safe_load(f)
            elif path.suffix == '.json':
                import json
                with open(path) as f:
                    file_config = json.load(f)
            else:
                logger.warning(f"Unknown config format: {path.suffix}")
                return
            
            if file_config:
                self._deep_merge(self.config, file_config)
                logger.info(f"Loaded config from: {config_path}")
                
        except Exception as e:
            logger.error(f"Failed to load config: {e}")
    
    def _load_default_paths(self):
        """Try loading from default configuration paths."""
        default_paths = [
            "config/config.yaml",
            "config/config.yml",
            "config/config.json",
            "config.yaml",
            "config.yml",
            "config.json",
            Path.home() / ".edr-framework" / "config.yaml",
        ]
        
        for path in default_paths:
            if Path(path).exists():
                self._load_file(str(path))
                break
    
    def _load_env_vars(self):
        """Load configuration from environment variables."""
        env_mapping = {
            "EDR_DEBUG": ("debug", lambda x: x.lower() == "true"),
            "EDR_LOG_LEVEL": ("log_level", str),
            "EDR_OUTPUT_DIR": ("output_dir", str),
            "EDR_AUTO_CLEANUP": ("agent.auto_cleanup", lambda x: x.lower() == "true"),
            "EDR_TELEMETRY_ENABLED": ("telemetry.enabled", lambda x: x.lower() == "true"),
            "EDR_SNAPSHOT_PROVIDER": ("snapshot.provider", str),
            "EDR_REPORT_FORMAT": ("reports.format", str),
        }
        
        for env_var, (config_key, converter) in env_mapping.items():
            value = os.environ.get(env_var)
            if value is not None:
                self.set(config_key, converter(value))
    
    def _deep_merge(self, base: Dict, override: Dict):
        """Deep merge override into base dictionary."""
        for key, value in override.items():
            if key in base and isinstance(base[key], dict) and isinstance(value, dict):
                self._deep_merge(base[key], value)
            else:
                base[key] = value
    
    def get(self, key: str, default: Any = None) -> Any:
        """
        Get configuration value.
        
        Args:
            key: Configuration key (supports dot notation for nested values)
            default: Default value if key not found
            
        Returns:
            Configuration value
        """
        keys = key.split(".")
        value = self.config
        
        for k in keys:
            if isinstance(value, dict) and k in value:
                value = value[k]
            else:
                return default
        
        return value
    
    def set(self, key: str, value: Any):
        """
        Set configuration value.
        
        Args:
            key: Configuration key (supports dot notation)
            value: Value to set
        """
        keys = key.split(".")
        config = self.config
        
        for k in keys[:-1]:
            if k not in config:
                config[k] = {}
            config = config[k]
        
        config[keys[-1]] = value
    
    def get_section(self, section: str) -> Dict[str, Any]:
        """
        Get a configuration section.
        
        Args:
            section: Section name
            
        Returns:
            Section dictionary
        """
        return self.config.get(section, {})
    
    def save(self, path: Optional[str] = None):
        """
        Save configuration to file.
        
        Args:
            path: Output path (uses original path if None)
        """
        path = path or self.config_path
        if not path:
            path = "config/config.yaml"
        
        output_path = Path(path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        try:
            if output_path.suffix in ['.yaml', '.yml']:
                import yaml
                with open(output_path, 'w') as f:
                    yaml.dump(self.config, f, default_flow_style=False)
            else:
                import json
                with open(output_path, 'w') as f:
                    json.dump(self.config, f, indent=2)
            
            logger.info(f"Saved config to: {output_path}")
            
        except Exception as e:
            logger.error(f"Failed to save config: {e}")
    
    def to_dict(self) -> Dict[str, Any]:
        """Return configuration as dictionary."""
        return self.config.copy()
    
    def __getitem__(self, key: str) -> Any:
        """Allow dict-style access."""
        return self.get(key)
    
    def __setitem__(self, key: str, value: Any):
        """Allow dict-style assignment."""
        self.set(key, value)
