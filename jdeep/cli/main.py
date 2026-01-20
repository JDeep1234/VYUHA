"""
CLI Main Entry Point
====================

Command-line interface for the EDR Adaptive Framework.
Uses Typer for modern CLI with auto-completion and help generation.
"""

import typer
from typing import Optional, List
from pathlib import Path
from rich.console import Console
from rich.table import Table
from rich.panel import Panel
from rich import print as rprint
import json
import yaml

app = typer.Typer(
    name="edr-framework",
    help="🛡️ EDR Adaptive Framework - Advanced APT Simulation & EDR Evasion Assessment",
    add_completion=True,
)

console = Console()


def print_banner():
    """Display the framework banner."""
    banner = """
    ╔═══════════════════════════════════════════════════════════════╗
    ║          🛡️  EDR ADAPTIVE FRAMEWORK v2.0  🛡️                  ║
    ║     Advanced APT Simulation & EDR Evasion Assessment          ║
    ╚═══════════════════════════════════════════════════════════════╝
    """
    console.print(Panel(banner, style="bold blue"))


@app.command()
def run(
    technique: str = typer.Argument(..., help="MITRE ATT&CK technique ID (e.g., T1055)"),
    target: str = typer.Option("localhost", "--target", "-t", help="Target system"),
    output: Optional[Path] = typer.Option(None, "--output", "-o", help="Output file path"),
    verbose: bool = typer.Option(False, "--verbose", "-v", help="Enable verbose output"),
    dry_run: bool = typer.Option(False, "--dry-run", help="Simulate without executing"),
):
    """
    Run a specific ATT&CK technique against the target.
    
    Example:
        edr-framework run T1055 --target localhost --verbose
    """
    print_banner()
    
    console.print(f"\n[bold green]▶ Running technique:[/] {technique}")
    console.print(f"[bold cyan]  Target:[/] {target}")
    
    if dry_run:
        console.print("[yellow]⚠ DRY RUN MODE - No actual execution[/]")
        return
    
    # Import and execute
    from jdeep.agent_core import AgentCore
    
    with console.status("[bold green]Initializing agent..."):
        agent = AgentCore(verbose=verbose)
    
    result = agent.execute_technique(technique, target)
    
    if output:
        with open(output, 'w') as f:
            json.dump(result, f, indent=2)
        console.print(f"\n[green]✓ Results saved to:[/] {output}")
    
    # Display results
    _display_result(result)


@app.command()
def campaign(
    config: Path = typer.Argument(..., help="Campaign configuration file (YAML/JSON)"),
    target: str = typer.Option("localhost", "--target", "-t", help="Target system"),
    output: Optional[Path] = typer.Option(None, "--output", "-o", help="Output directory"),
    parallel: bool = typer.Option(False, "--parallel", "-p", help="Run techniques in parallel"),
):
    """
    Execute a full attack campaign from configuration.
    
    Example:
        edr-framework campaign campaigns/apt29.yaml --target 192.168.1.100
    """
    print_banner()
    
    console.print(f"\n[bold green]▶ Loading campaign:[/] {config}")
    
    # Load campaign config
    with open(config) as f:
        if config.suffix == '.yaml' or config.suffix == '.yml':
            campaign_config = yaml.safe_load(f)
        else:
            campaign_config = json.load(f)
    
    console.print(f"[bold cyan]  Campaign Name:[/] {campaign_config.get('name', 'Unknown')}")
    console.print(f"[bold cyan]  Techniques:[/] {len(campaign_config.get('techniques', []))}")
    
    from jdeep.agent_core import AgentCore
    
    agent = AgentCore()
    results = agent.execute_campaign(campaign_config, target, parallel=parallel)
    
    if output:
        output.mkdir(parents=True, exist_ok=True)
        results_file = output / "campaign_results.json"
        with open(results_file, 'w') as f:
            json.dump(results, f, indent=2)
        console.print(f"\n[green]✓ Results saved to:[/] {results_file}")


@app.command()
def list_techniques(
    tactic: Optional[str] = typer.Option(None, "--tactic", "-t", help="Filter by tactic"),
    search: Optional[str] = typer.Option(None, "--search", "-s", help="Search techniques"),
):
    """
    List available ATT&CK techniques.
    
    Example:
        edr-framework list-techniques --tactic execution
    """
    print_banner()
    
    # Define available techniques (will be expanded with actual technique loader)
    techniques = [
        {"id": "T1055", "name": "Process Injection", "tactic": "Defense Evasion"},
        {"id": "T1055.001", "name": "DLL Injection", "tactic": "Defense Evasion"},
        {"id": "T1055.012", "name": "Process Hollowing", "tactic": "Defense Evasion"},
        {"id": "T1218.001", "name": "Compiled HTML File", "tactic": "Defense Evasion"},
        {"id": "T1218.005", "name": "Mshta", "tactic": "Defense Evasion"},
        {"id": "T1218.011", "name": "Rundll32", "tactic": "Defense Evasion"},
        {"id": "T1574.001", "name": "DLL Search Order Hijacking", "tactic": "Defense Evasion"},
        {"id": "T1574.002", "name": "DLL Side-Loading", "tactic": "Defense Evasion"},
        {"id": "T1003.001", "name": "LSASS Memory", "tactic": "Credential Access"},
        {"id": "T1558.003", "name": "Kerberoasting", "tactic": "Credential Access"},
        {"id": "T1059.001", "name": "PowerShell", "tactic": "Execution"},
        {"id": "T1047", "name": "WMI", "tactic": "Execution"},
        {"id": "T1053.005", "name": "Scheduled Task", "tactic": "Persistence"},
        {"id": "T1543.003", "name": "Windows Service", "tactic": "Persistence"},
        {"id": "T1548.002", "name": "UAC Bypass", "tactic": "Privilege Escalation"},
        {"id": "T1134", "name": "Access Token Manipulation", "tactic": "Privilege Escalation"},
        {"id": "T1021.001", "name": "RDP", "tactic": "Lateral Movement"},
        {"id": "T1021.002", "name": "SMB/Windows Admin Shares", "tactic": "Lateral Movement"},
        {"id": "T1071.001", "name": "Web Protocols", "tactic": "Command and Control"},
        {"id": "T1071.004", "name": "DNS", "tactic": "Command and Control"},
    ]
    
    # Filter
    if tactic:
        techniques = [t for t in techniques if tactic.lower() in t["tactic"].lower()]
    if search:
        techniques = [t for t in techniques if search.lower() in t["name"].lower() or search.upper() in t["id"]]
    
    # Display table
    table = Table(title="Available Techniques", show_header=True, header_style="bold magenta")
    table.add_column("ID", style="cyan", width=12)
    table.add_column("Name", style="green")
    table.add_column("Tactic", style="yellow")
    
    for t in techniques:
        table.add_row(t["id"], t["name"], t["tactic"])
    
    console.print(table)
    console.print(f"\n[bold]Total:[/] {len(techniques)} techniques")


@app.command()
def report(
    results: Path = typer.Argument(..., help="Results file or directory"),
    format: str = typer.Option("html", "--format", "-f", help="Output format: html, pdf, json, stix"),
    output: Optional[Path] = typer.Option(None, "--output", "-o", help="Output file path"),
):
    """
    Generate reports from test results.
    
    Example:
        edr-framework report results/campaign.json --format pdf --output report.pdf
    """
    print_banner()
    
    console.print(f"\n[bold green]▶ Generating report from:[/] {results}")
    console.print(f"[bold cyan]  Format:[/] {format}")
    
    from jdeep.agent_core.output_handler import OutputHandler
    
    handler = OutputHandler()
    report_path = handler.generate_report(results, format, output)
    
    console.print(f"\n[green]✓ Report generated:[/] {report_path}")


@app.command()
def status():
    """
    Show framework status and configuration.
    """
    print_banner()
    
    from jdeep.agent_core import AgentCore
    from jdeep.integration import IntegrationManager
    
    # Framework status
    table = Table(title="Framework Status", show_header=True)
    table.add_column("Component", style="cyan")
    table.add_column("Status", style="green")
    table.add_column("Details", style="yellow")
    
    table.add_row("CLI Tool", "✓ Active", "v2.0.0")
    table.add_row("Agent Core", "✓ Ready", "All modules loaded")
    table.add_row("Integration Module", "✓ Ready", "EDR connectors available")
    table.add_row("Configuration", "✓ Loaded", "config/config.yaml")
    
    console.print(table)
    
    # EDR Status
    console.print("\n")
    integration = IntegrationManager()
    edr_status = integration.get_edr_status()
    
    edr_table = Table(title="EDR/AV Detection Status", show_header=True)
    edr_table.add_column("Product", style="cyan")
    edr_table.add_column("Status", style="green")
    edr_table.add_column("Version", style="yellow")
    
    for edr in edr_status:
        status_icon = "✓ Running" if edr.get("running") else "✗ Not Found"
        edr_table.add_row(edr["name"], status_icon, edr.get("version", "N/A"))
    
    console.print(edr_table)


@app.command()
def snapshot(
    action: str = typer.Argument(..., help="Action: create, restore, list, delete"),
    name: Optional[str] = typer.Option(None, "--name", "-n", help="Snapshot name"),
    vm: Optional[str] = typer.Option(None, "--vm", help="Virtual machine name"),
):
    """
    Manage VM snapshots for clean testing environments.
    
    Example:
        edr-framework snapshot create --name "clean-state" --vm "Windows10-Test"
        edr-framework snapshot restore --name "clean-state" --vm "Windows10-Test"
    """
    print_banner()
    
    from jdeep.integration.snapshot_manager import SnapshotManager
    
    manager = SnapshotManager()
    
    if action == "create":
        if not name or not vm:
            console.print("[red]Error: --name and --vm are required for create[/]")
            raise typer.Exit(1)
        manager.create_snapshot(vm, name)
        console.print(f"[green]✓ Snapshot '{name}' created for VM '{vm}'[/]")
        
    elif action == "restore":
        if not name or not vm:
            console.print("[red]Error: --name and --vm are required for restore[/]")
            raise typer.Exit(1)
        manager.restore_snapshot(vm, name)
        console.print(f"[green]✓ VM '{vm}' restored to snapshot '{name}'[/]")
        
    elif action == "list":
        snapshots = manager.list_snapshots(vm)
        table = Table(title="Available Snapshots")
        table.add_column("VM", style="cyan")
        table.add_column("Snapshot", style="green")
        table.add_column("Created", style="yellow")
        for snap in snapshots:
            table.add_row(snap["vm"], snap["name"], snap["created"])
        console.print(table)
        
    elif action == "delete":
        if not name or not vm:
            console.print("[red]Error: --name and --vm are required for delete[/]")
            raise typer.Exit(1)
        manager.delete_snapshot(vm, name)
        console.print(f"[green]✓ Snapshot '{name}' deleted from VM '{vm}'[/]")


@app.command()
def clean(
    target: str = typer.Option("localhost", "--target", "-t", help="Target system"),
    artifacts: bool = typer.Option(True, "--artifacts/--no-artifacts", help="Clean file artifacts"),
    registry: bool = typer.Option(True, "--registry/--no-registry", help="Clean registry entries"),
    processes: bool = typer.Option(False, "--processes", "-p", help="Kill spawned processes"),
):
    """
    Clean up artifacts from previous test runs.
    
    Example:
        edr-framework clean --target localhost --artifacts --registry
    """
    print_banner()
    
    from jdeep.agent_core.cleaner import ArtifactCleaner
    
    cleaner = ArtifactCleaner()
    
    console.print(f"\n[bold green]▶ Cleaning artifacts on:[/] {target}")
    
    results = cleaner.clean(
        target=target,
        clean_artifacts=artifacts,
        clean_registry=registry,
        kill_processes=processes
    )
    
    console.print(f"\n[green]✓ Cleanup complete[/]")
    console.print(f"  Files removed: {results.get('files_removed', 0)}")
    console.print(f"  Registry keys cleaned: {results.get('registry_cleaned', 0)}")
    if processes:
        console.print(f"  Processes terminated: {results.get('processes_killed', 0)}")


def _display_result(result: dict):
    """Display execution result in a formatted table."""
    table = Table(title="Execution Result", show_header=True)
    table.add_column("Field", style="cyan")
    table.add_column("Value", style="green")
    
    table.add_row("Technique", result.get("technique", "N/A"))
    table.add_row("Status", "✓ Success" if result.get("success") else "✗ Failed")
    table.add_row("Detected", "Yes" if result.get("detected") else "No")
    table.add_row("Evasion Score", f"{result.get('evasion_score', 0):.2f}")
    table.add_row("Duration", f"{result.get('duration', 0):.2f}s")
    
    console.print(table)
    
    if result.get("alerts"):
        console.print("\n[bold yellow]⚠ EDR Alerts:[/]")
        for alert in result["alerts"]:
            console.print(f"  - {alert}")


def main():
    """Main entry point."""
    app()


if __name__ == "__main__":
    main()
