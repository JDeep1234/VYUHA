"""
EDR Adaptive Framework - Jdeep's Components
============================================

This package contains the core components developed by Jdeep:
- CLI Tool: Command-line interface for the framework
- Agent Core: Orchestrator, Output Handler, Cleaner, Telemetry Monitor
- Integration Module: EDR Integration, Snapshot Manager, Clean Module

Author: Jdeep
"""

__version__ = "2.0.0"
__author__ = "Jdeep"

from jdeep.cli import main as cli_main
from jdeep.agent_core import AgentCore
from jdeep.integration import IntegrationManager

__all__ = ["cli_main", "AgentCore", "IntegrationManager"]
