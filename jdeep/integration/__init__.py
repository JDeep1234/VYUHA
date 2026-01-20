"""
Integration Module
==================

Handles integration with external systems:
- EDR Integration: Connectors for CrowdStrike, Defender, etc.
- Snapshot Manager: VM state management
- Clean Module: System restoration
"""

from jdeep.integration.edr_connector import EDRConnector
from jdeep.integration.snapshot_manager import SnapshotManager
from jdeep.integration.clean_module import CleanModule
from jdeep.integration.manager import IntegrationManager

__all__ = [
    "IntegrationManager",
    "EDRConnector",
    "SnapshotManager",
    "CleanModule",
]
