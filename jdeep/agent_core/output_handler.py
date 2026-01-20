"""
Output Handler Module
=====================

Handles result formatting, logging, and export to various formats.
Supports JSON, XML, CSV, HTML, PDF, and STIX formats.
"""

import json
import logging
import csv
from typing import Dict, Any, List, Optional
from pathlib import Path
from datetime import datetime

logger = logging.getLogger(__name__)


class OutputHandler:
    """
    Handles output formatting and export for test results.
    
    Supports multiple output formats:
    - JSON: Structured data export
    - CSV: Spreadsheet-compatible format
    - HTML: Human-readable reports
    - PDF: Formal documentation
    - STIX: Threat intelligence format
    """
    
    def __init__(self, config=None):
        """
        Initialize the Output Handler.
        
        Args:
            config: ConfigManager instance
        """
        self.config = config
        self.results_buffer: List[Dict[str, Any]] = []
        self.output_dir = Path(config.get("output_dir", "results") if config else "results")
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        logger.info(f"OutputHandler initialized - Output dir: {self.output_dir}")
    
    def log_result(self, result: Dict[str, Any]):
        """
        Log a single execution result.
        
        Args:
            result: Execution result dictionary
        """
        self.results_buffer.append(result)
        
        # Log to console
        status = "✓" if result.get("success") else "✗"
        detected = "DETECTED" if result.get("detected") else "EVADED"
        logger.info(
            f"[{status}] {result.get('technique')} - {detected} "
            f"(score: {result.get('evasion_score', 0):.2f})"
        )
    
    def export_json(self, results: List[Dict[str, Any]], output_path: Path) -> Path:
        """
        Export results to JSON format.
        
        Args:
            results: List of result dictionaries
            output_path: Output file path
            
        Returns:
            Path to exported file
        """
        with open(output_path, 'w') as f:
            json.dump({
                "export_time": datetime.now().isoformat(),
                "total_results": len(results),
                "results": results
            }, f, indent=2, default=str)
        
        logger.info(f"Exported JSON: {output_path}")
        return output_path
    
    def export_csv(self, results: List[Dict[str, Any]], output_path: Path) -> Path:
        """
        Export results to CSV format.
        
        Args:
            results: List of result dictionaries
            output_path: Output file path
            
        Returns:
            Path to exported file
        """
        if not results:
            return output_path
        
        fieldnames = [
            "technique", "target", "success", "detected", 
            "evasion_score", "duration", "timestamp", "error"
        ]
        
        with open(output_path, 'w', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction='ignore')
            writer.writeheader()
            writer.writerows(results)
        
        logger.info(f"Exported CSV: {output_path}")
        return output_path
    
    def export_html(self, results: List[Dict[str, Any]], output_path: Path) -> Path:
        """
        Export results to HTML format.
        
        Args:
            results: List of result dictionaries
            output_path: Output file path
            
        Returns:
            Path to exported file
        """
        html_template = """<!DOCTYPE html>
<html>
<head>
    <title>EDR Framework - Test Results</title>
    <style>
        body {{ font-family: 'Segoe UI', Arial, sans-serif; margin: 40px; background: #f5f5f5; }}
        .header {{ background: linear-gradient(135deg, #1a1a2e, #16213e); color: white; padding: 30px; border-radius: 10px; margin-bottom: 30px; }}
        .header h1 {{ margin: 0; font-size: 28px; }}
        .header p {{ margin: 10px 0 0 0; opacity: 0.8; }}
        .summary {{ display: grid; grid-template-columns: repeat(4, 1fr); gap: 20px; margin-bottom: 30px; }}
        .summary-card {{ background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }}
        .summary-card h3 {{ margin: 0 0 10px 0; color: #666; font-size: 14px; }}
        .summary-card .value {{ font-size: 36px; font-weight: bold; color: #1a1a2e; }}
        table {{ width: 100%; border-collapse: collapse; background: white; border-radius: 10px; overflow: hidden; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }}
        th {{ background: #1a1a2e; color: white; padding: 15px; text-align: left; }}
        td {{ padding: 15px; border-bottom: 1px solid #eee; }}
        tr:hover {{ background: #f9f9f9; }}
        .success {{ color: #28a745; font-weight: bold; }}
        .failed {{ color: #dc3545; font-weight: bold; }}
        .detected {{ background: #fff3cd; color: #856404; padding: 3px 8px; border-radius: 4px; }}
        .evaded {{ background: #d4edda; color: #155724; padding: 3px 8px; border-radius: 4px; }}
        .score {{ font-weight: bold; }}
        .score-high {{ color: #28a745; }}
        .score-mid {{ color: #ffc107; }}
        .score-low {{ color: #dc3545; }}
    </style>
</head>
<body>
    <div class="header">
        <h1>🛡️ EDR Adaptive Framework - Test Results</h1>
        <p>Generated: {timestamp}</p>
    </div>
    
    <div class="summary">
        <div class="summary-card">
            <h3>Total Tests</h3>
            <div class="value">{total}</div>
        </div>
        <div class="summary-card">
            <h3>Successful</h3>
            <div class="value" style="color: #28a745;">{successful}</div>
        </div>
        <div class="summary-card">
            <h3>Detected</h3>
            <div class="value" style="color: #dc3545;">{detected}</div>
        </div>
        <div class="summary-card">
            <h3>Avg Evasion Score</h3>
            <div class="value">{avg_score:.2f}</div>
        </div>
    </div>
    
    <table>
        <thead>
            <tr>
                <th>Technique</th>
                <th>Target</th>
                <th>Status</th>
                <th>Detection</th>
                <th>Evasion Score</th>
                <th>Duration</th>
            </tr>
        </thead>
        <tbody>
            {rows}
        </tbody>
    </table>
</body>
</html>"""
        
        # Generate rows
        rows_html = ""
        for r in results:
            score = r.get("evasion_score", 0)
            score_class = "score-high" if score >= 0.7 else "score-mid" if score >= 0.4 else "score-low"
            
            rows_html += f"""
            <tr>
                <td><strong>{r.get('technique', 'N/A')}</strong></td>
                <td>{r.get('target', 'N/A')}</td>
                <td class="{'success' if r.get('success') else 'failed'}">{'✓ Success' if r.get('success') else '✗ Failed'}</td>
                <td><span class="{'detected' if r.get('detected') else 'evaded'}">{'Detected' if r.get('detected') else 'Evaded'}</span></td>
                <td class="score {score_class}">{score:.2f}</td>
                <td>{r.get('duration', 0):.2f}s</td>
            </tr>"""
        
        # Calculate summary
        total = len(results)
        successful = sum(1 for r in results if r.get("success"))
        detected = sum(1 for r in results if r.get("detected"))
        avg_score = sum(r.get("evasion_score", 0) for r in results) / total if total else 0
        
        html = html_template.format(
            timestamp=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            total=total,
            successful=successful,
            detected=detected,
            avg_score=avg_score,
            rows=rows_html
        )
        
        with open(output_path, 'w') as f:
            f.write(html)
        
        logger.info(f"Exported HTML: {output_path}")
        return output_path
    
    def export_stix(self, results: List[Dict[str, Any]], output_path: Path) -> Path:
        """
        Export results to STIX 2.1 format.
        
        Args:
            results: List of result dictionaries
            output_path: Output file path
            
        Returns:
            Path to exported file
        """
        stix_bundle = {
            "type": "bundle",
            "id": f"bundle--{datetime.now().strftime('%Y%m%d%H%M%S')}",
            "objects": []
        }
        
        # Create attack-pattern objects for each technique
        for r in results:
            attack_pattern = {
                "type": "attack-pattern",
                "spec_version": "2.1",
                "id": f"attack-pattern--{r.get('technique', 'unknown')}",
                "name": r.get("technique", "Unknown Technique"),
                "description": f"Test execution of {r.get('technique')}",
                "external_references": [
                    {
                        "source_name": "mitre-attack",
                        "external_id": r.get("technique", ""),
                        "url": f"https://attack.mitre.org/techniques/{r.get('technique', '').replace('.', '/')}"
                    }
                ],
                "x_edr_framework_result": {
                    "success": r.get("success"),
                    "detected": r.get("detected"),
                    "evasion_score": r.get("evasion_score"),
                    "target": r.get("target"),
                    "timestamp": r.get("timestamp")
                }
            }
            stix_bundle["objects"].append(attack_pattern)
        
        with open(output_path, 'w') as f:
            json.dump(stix_bundle, f, indent=2)
        
        logger.info(f"Exported STIX: {output_path}")
        return output_path
    
    def generate_report(
        self,
        results_source: Path,
        format: str = "html",
        output_path: Optional[Path] = None
    ) -> Path:
        """
        Generate a report from results.
        
        Args:
            results_source: Path to results file or directory
            format: Output format (html, pdf, json, stix)
            output_path: Output file path
            
        Returns:
            Path to generated report
        """
        # Load results
        if results_source.is_file():
            with open(results_source) as f:
                data = json.load(f)
                results = data.get("results", []) if isinstance(data, dict) else data
        else:
            # Load all JSON files from directory
            results = []
            for json_file in results_source.glob("*.json"):
                with open(json_file) as f:
                    data = json.load(f)
                    if isinstance(data, list):
                        results.extend(data)
                    elif "results" in data:
                        results.extend(data["results"])
        
        # Determine output path
        if not output_path:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            output_path = self.output_dir / f"report_{timestamp}.{format}"
        
        # Export based on format
        if format == "html":
            return self.export_html(results, output_path)
        elif format == "json":
            return self.export_json(results, output_path)
        elif format == "csv":
            return self.export_csv(results, output_path)
        elif format == "stix":
            return self.export_stix(results, output_path)
        elif format == "pdf":
            # PDF requires additional dependencies
            logger.warning("PDF export requires weasyprint. Falling back to HTML.")
            return self.export_html(results, output_path.with_suffix('.html'))
        else:
            raise ValueError(f"Unknown format: {format}")
    
    def flush(self, format: str = "json") -> Path:
        """
        Flush buffered results to file.
        
        Args:
            format: Output format
            
        Returns:
            Path to output file
        """
        if not self.results_buffer:
            logger.warning("No results to flush")
            return None
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_path = self.output_dir / f"results_{timestamp}.{format}"
        
        if format == "json":
            self.export_json(self.results_buffer, output_path)
        elif format == "csv":
            self.export_csv(self.results_buffer, output_path)
        elif format == "html":
            self.export_html(self.results_buffer, output_path)
        
        self.results_buffer.clear()
        return output_path
