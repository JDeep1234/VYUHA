# OT-Sentinel: Frequently Asked Questions (FAQ)

This document provides comprehensive answers to common questions about the OT-Sentinel project - an AI-powered Industrial Control Systems (ICS) security dashboard with real-time threat detection for SCADA, PLCs & OT networks.

**GitHub Repository:** https://github.com/BipinRajC/OT-Sentinel

---

## Table of Contents
1. [How the Data is Stored](#1-how-the-data-is-stored)
2. [Lab Setup - Sensors and PLC Connection](#2-lab-setup---sensors-and-plc-connection)
3. [How the Interaction Takes Place](#3-how-the-interaction-takes-place)
4. [Where Sensor Data is Stored](#4-where-sensor-data-is-stored)
5. [Network Topology Creation and Alert Generation](#5-network-topology-creation-and-alert-generation)
6. [Vulnerabilities from Datasets (SWaT, BATADAL, WADI)](#6-vulnerabilities-from-datasets-swat-batadal-wadi)
7. [Dataset Usage and Real-Time Industry Adaptation](#7-dataset-usage-and-real-time-industry-adaptation)
8. [Understanding TenEast Chemical Process & Simulation](#8-understanding-teneast-chemical-process--simulation)

---

## 1. How the Data is Stored

### Data Storage Architecture

OT-Sentinel uses a **multi-layered data storage approach**:

#### a) **CSV Dataset Files (Primary Storage)**
The main dataset is stored in CSV format at:
```
trained_models/
├── balanced_subset.csv              # Training dataset subset for simulation
├── processed_ics_dataset_cleaned.csv # Cleaned ICS network dataset
├── ensemble_model.pkl               # Trained ML classification model
├── scalers.pkl                      # Feature preprocessing parameters
├── label_encoder.pkl                # Attack type label encoding
└── feature_selectors.pkl            # Feature selection parameters
```

#### b) **PostgreSQL Database (Production)**
For production environments, data is stored in PostgreSQL:
```python
# Database Configuration from .env file
POSTGRES_USER=icsuser
POSTGRES_PASSWORD=icspassword
POSTGRES_DB=ics_security
DATABASE_URL=postgresql://icsuser:icspassword@postgres:5432/ics_security
```

The `network_traffic` table schema:
```sql
CREATE TABLE network_traffic (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    src_ip VARCHAR(15),
    dst_ip VARCHAR(15),
    protocol VARCHAR(10),
    packet_size INTEGER,
    label VARCHAR(20),
    category VARCHAR(10),
    feature_0 FLOAT, feature_1 FLOAT, ... feature_19 FLOAT
);
```

#### c) **Redis Cache (Real-Time Data)**
Redis is used for caching real-time data and WebSocket message brokering:
```
REDIS_URL=redis://redis:6379/0
```

#### d) **In-Memory Storage (Runtime)**
- **Device Data**: Stored in `devices_data` list in the API
- **Alerts Data**: Stored in `alerts_data` list
- **Recent Classifications**: Stored in `recent_classifications` deque (max 1000 entries)

---

## 2. Lab Setup - Sensors and PLC Connection

### Industrial Process Configuration

The lab setup simulates a **TenEast Chemical Process** plant using the following architecture:

#### a) **Device Hierarchy**

| Device Type | IP Address | Protocol | Port | Description |
|-------------|-----------|----------|------|-------------|
| **PLC (Process Controller)** | 192.168.95.2 | Modbus TCP | 502 | Main TenEast Process Controller |
| **HMI Station** | 192.168.95.3 | EtherNet/IP | 44818 | Human-Machine Interface |
| **SCADA Server** | 192.168.95.4 | DNP3 | 20000 | Supervisory Control System |
| **RTU** | 192.168.95.5 | Modbus RTU | 502 | Remote Terminal Unit |
| **Historian Server** | 192.168.95.6 | OPC UA | 4840 | Data Historian |
| **Safety PLC** | 192.168.95.7 | EtherNet/IP | 44818 | Safety Controller |
| **I/O Module Rack** | 192.168.95.11 | Modbus TCP | 502 | Field I/O Module |
| **VFD Panel** | 192.168.95.12 | Modbus RTU | 502 | Variable Frequency Drive |

#### b) **Sensor/Process Points Configuration**

The industrial process includes these simulated sensors:

```python
process_points = [
    {"name": "AValve", "type": "valve_position", "unit": "%", "register": 0},
    {"name": "ProductValve", "type": "valve_position", "unit": "%"},
    {"name": "Feed2", "type": "flow_rate", "unit": "GPM"},
    {"name": "Feed1", "type": "flow_rate", "unit": "GPM"},
    {"name": "Pressure", "type": "pressure", "unit": "PSI"},
    {"name": "Level", "type": "level", "unit": "%"},
    {"name": "Composition", "type": "composition", "unit": "%"},
    # ... and more process control points
]
```

#### c) **Physical Connection Architecture**

```mermaid
flowchart TB
    subgraph CONTROL["🏢 CONTROL CENTER"]
        SCADA["📡 SCADA Server<br/>192.168.95.4"]
        HMI["🖥️ HMI Station<br/>192.168.95.3"]
        HISTORIAN["💾 Historian<br/>192.168.95.6"]
    end

    SCADA & HMI & HISTORIAN --> SWITCH["🔀 Industrial Switch"]

    subgraph FIELD["⚙️ FIELD LAYER"]
        PLC["🎛️ PLC<br/>192.168.95.2"]
        RTU["📟 RTU<br/>192.168.95.5"]
        SAFETY["🛡️ Safety PLC<br/>192.168.95.7"]
    end

    SWITCH --> PLC & RTU & SAFETY

    subgraph DEVICES["🔧 FIELD DEVICES"]
        IO["📊 I/O Rack<br/>Field Sensors"]
        VFD["⚡ VFD Panel"]
        SENSORS["🌡️ Safety Sensors"]
    end

    PLC --> IO
    RTU --> VFD
    SAFETY --> SENSORS

    style CONTROL fill:#1e3a5f,stroke:#3b82f6,color:#fff
    style FIELD fill:#1e3a5f,stroke:#f97316,color:#fff
    style DEVICES fill:#1e3a5f,stroke:#10b981,color:#fff
    style SWITCH fill:#6366f1,stroke:#818cf8,color:#fff
```

#### d) **Modbus TCP Communication**

The PLC communicates via Modbus TCP with these settings:
```python
MODBUS_CONFIG = {
    "host": "192.168.95.2",
    "port": 502,
    "slave_id": 1,
    "update_period": 200,  # milliseconds
    "max_read_register_count": 125,
    "max_write_register_count": 120,
    "timeout": 500
}
```

---

## 3. How the Interaction Takes Place

### System Interaction Flow

#### a) **Data Flow Architecture**

```mermaid
flowchart LR
    subgraph DATA["📁 Data Layer"]
        CSV[("📄 CSV Dataset")]
        DB[("🗄️ PostgreSQL")]
    end

    subgraph BACKEND["⚙️ Backend (FastAPI)"]
        API["🔌 REST API"]
        ML["🤖 ML Models<br/>(Ensemble)"]
    end

    subgraph FRONTEND["🖥️ Frontend (React)"]
        DASH["📊 Dashboard"]
        WS["📡 WebSocket<br/>Real-time"]
    end

    CSV --> API
    DB --> API
    API --> ML
    API --> DASH
    ML --> WS
    WS --> DASH

    style DATA fill:#0f172a,stroke:#3b82f6,color:#fff
    style BACKEND fill:#0f172a,stroke:#8b5cf6,color:#fff
    style FRONTEND fill:#0f172a,stroke:#10b981,color:#fff
```

#### b) **Step-by-Step Interaction**

1. **Dataset Loading**
   ```python
   # Service loads CSV dataset with timestamp column
   self.dataset = pd.read_csv(self.dataset_path)
   self.total_packets = len(self.dataset)
   ```

2. **Progressive Reading**
   - Reads data chunks in timestamp order
   - Uses random sampling for varied traffic patterns

3. **Preprocessing**
   - Applies same preprocessing as training pipeline
   - Feature scaling using saved scalers
   ```python
   if self.scalers:
       features = self.scalers.transform(features.reshape(1, -1))
   ```

4. **ML Inference**
   - Ensemble model classifies each packet
   - Returns attack type and confidence score
   ```python
   prediction = self.model.predict(features)
   confidence = self.model.predict_proba(features).max()
   ```

5. **Real-time Broadcasting**
   - Results streamed via WebSocket
   ```python
   await websocket.send_text(json.dumps({
       "type": "classification",
       "data": classification_result
   }))
   ```

6. **Visualization Updates**
   - Frontend receives real-time updates
   - Charts and tables update dynamically

#### c) **API Endpoints Interaction**

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/devices` | GET | Fetch all ICS devices |
| `/api/alerts` | GET | Fetch security alerts |
| `/api/network/topology` | GET | Get network topology |
| `/api/traffic/realtime` | GET | Real-time traffic data |
| `/api/realtime/start` | POST | Start simulation |
| `/api/realtime/stop` | POST | Stop simulation |
| `/ws` | WebSocket | Real-time classifications |

#### d) **WebSocket Message Types**

```javascript
// Classification result
{
    "type": "classification",
    "data": {
        "timestamp": "2024-01-15T10:30:00Z",
        "source_ip": "192.168.1.50",
        "destination_ip": "192.168.1.100",
        "predicted_class": "dos",
        "confidence": 0.95,
        "severity": "high"
    }
}

// Statistics update
{
    "type": "statistics",
    "data": {
        "total_packets": 1500,
        "attack_distribution": {...}
    }
}
```

---

## 4. Where Sensor Data is Stored

### Sensor Data Storage Locations

#### a) **Industrial Process Data Generator**

Sensor data is primarily generated and stored in-memory via the `IndustrialDataGenerator` class:

```python
class IndustrialDataGenerator:
    """Generate realistic industrial process data"""
    
    def get_current_data(self):
        """Get current values for all process points"""
        current_data = []
        for point in INDUSTRIAL_CONFIG["process_points"]:
            current_data.append({
                "id": point["xid"],
                "name": point["name"],
                "value": self._generate_value(point),
                "unit": point["unit"],
                "timestamp": datetime.utcnow().isoformat(),
                "alarm_status": self._check_alarm(point)
            })
        return current_data
```

#### b) **Storage Hierarchy**

| Data Type | Storage Location | Persistence |
|-----------|-----------------|-------------|
| **Live Process Values** | Python Memory (IndustrialDataGenerator) | Volatile |
| **Historical Network Traffic** | PostgreSQL `network_traffic` table | Persistent |
| **Training Dataset** | CSV files (`trained_models/`) | Persistent |
| **Alert History** | `alerts_data` in-memory list | Volatile |
| **Session Cache** | Redis | Semi-persistent |
| **Classification Results** | `recent_classifications` deque | Volatile (max 1000) |

#### c) **Historian Integration**

For production deployments, sensor data flows to the Historian Server:
```python
HISTORIAN_CONFIG = {
    "device": "Historian Server",
    "ip": "192.168.95.6",
    "protocol": "OPC UA",
    "port": 4840,
    "process_points": 500  # Maximum data points stored
}
```

#### d) **Data Point Storage Format**

```python
# Example stored process point
{
    "xid": "DP_909767",
    "name": "AValve",
    "description": "Feed1 Control Valve A",
    "value": 75.5,
    "unit": "%",
    "type": "valve_position",
    "normal_range": [0, 100],
    "critical_threshold": 95,
    "modbus_register": 0,
    "timestamp": "2024-01-15T10:30:00.000Z",
    "alarm_status": "normal"
}
```

---

## 5. Network Topology Creation and Alert Generation

### A. Network Topology Creation

#### a) **Topology Data Source**

The network topology is created from the `devices_data` configuration and served via API:

```python
@app.get("/api/network/topology")
def get_network_topology():
    """Get network topology information"""
    # Build topology from device relationships
    topology = {
        "nodes": [],      # Device nodes
        "edges": [],      # Connections between devices
        "metrics": {}     # Network health metrics
    }
```

#### b) **Topology Structure**

```python
# Network map connections
connections = []

# Connect critical devices to industrial switch
switch_id = "industrial_switch"
critical_devices = ["PLC", "HMI", "SCADA", "Safety PLC"]

for device_id in critical_devices:
    connections.append({
        "id": f"conn_switch_{device_id}",
        "source": switch_id,
        "target": device_id,
        "protocol": "EtherNet/IP",
        "status": "active",
        "bandwidth": random.randint(500, 800)
    })

# Connect field devices to PLC
field_devices = ["RTU", "I/O Module", "VFD"]
for device in field_devices:
    connections.append({
        "source": "PLC",
        "target": device,
        "protocol": "Modbus TCP"
    })
```

#### c) **Topology Visualization Components**

The frontend displays topology using:
- **React Network Graph** - Interactive node visualization
- **Device Status Colors**:
  - 🟢 Green: Online
  - 🟡 Yellow: Warning
  - 🔴 Red: Offline/Under Attack

```javascript
const getDeviceColor = (type) => {
    switch (type) {
        case 'hmi': return '#00bcd4';
        case 'plc': return '#ff9800';
        case 'rtu': return '#4caf50';
        case 'scada': return '#9c27b0';
        case 'historian': return '#f44336';
        default: return '#757575';
    }
};
```

### B. Alert Generation

#### a) **Alert Types**

| Alert Type | Severity | Description |
|------------|----------|-------------|
| Buffer Overflow Attempt | Critical | Exploitation of libmodbus vulnerability |
| Unauthorized Access | High | Failed authentication attempts |
| Network Anomaly | Medium | Unusual traffic patterns |
| Protocol Violation | Low | Modbus function code violations |
| Device Offline | Medium | Lost connection to device |
| Malware Detected | Critical | Malicious code detected |

#### b) **Alert Generation Mechanisms**

**1. ML-Based Detection:**
```python
def classify_packet(self, packet):
    prediction = self.model.predict(features)
    
    if prediction != 'normal':
        alert = {
            "type": prediction,
            "severity": self._get_severity(prediction),
            "source_ip": packet.source_ip,
            "confidence": confidence_score
        }
        self.generate_alert(alert)
```

**2. Process Threshold Alerts:**
```python
def generate_alerts(self):
    alerts = []
    for point in current_data:
        if point["alarm_status"] == "critical":
            alerts.append({
                "alert_type": f"Critical {point['type']}",
                "severity": "critical",
                "description": f"{point['description']} exceeded threshold"
            })
```

**3. Real-time Event Generation:**
```python
# Background task generates events
alert_types = [
    "Buffer Overflow Attempt",
    "Unauthorized Access",
    "Malware Detected",
    "Network Anomaly",
    "Protocol Violation"
]

new_alert = {
    "id": len(alerts_data) + 1,
    "device_id": device['id'],
    "alert_type": random.choice(alert_types),
    "severity": random.choice(["low", "medium", "high", "critical"]),
    "timestamp": datetime.utcnow().isoformat()
}
```

#### c) **Alert Storage**

Alerts are stored in the `alerts_data` list and broadcast via WebSocket:
```python
# Store alert
alerts_data.append(new_alert)

# Broadcast to all connected clients
for connection in active_connections:
    await connection.send_json({
        "type": "new_alert",
        "alert": new_alert
    })
```

---

## 6. Vulnerabilities from Datasets (SWaT, BATADAL, WADI)

### Dataset-Based Vulnerability Coverage

The OT-Sentinel system is trained on ICS security datasets and covers the following vulnerability categories:

#### a) **Attack Types Detected**

| Attack Type | Description | Detection Accuracy |
|-------------|-------------|-------------------|
| **Normal Traffic** | Baseline network behavior | 99.2% |
| **DoS (Denial of Service)** | Flooding attacks on ICS networks | 99.1% |
| **MITM Attacks** | Man-in-the-Middle interception | 98.7% |
| **Modbus Flooding** | Industrial protocol DoS | 99.1% |
| **TCP SYN Flood** | Network layer DDoS | 98.9% |
| **ICMP Flood** | Ping flood attacks | 99.0% |
| **Probe/Reconnaissance** | Network scanning activities | ~95% |
| **R2L (Remote to Local)** | Remote exploitation attempts | ~94% |
| **U2R (User to Root)** | Privilege escalation | ~93% |

#### b) **Vulnerabilities Based on SWaT Dataset**

The **Secure Water Treatment (SWaT)** dataset covers:

1. **Physical Process Attacks:**
   - Manipulating valve positions
   - Altering flow rates
   - Tank level manipulation
   - Pump control attacks

2. **Network-Level Attacks:**
   - Command injection via Modbus
   - Replay attacks on PLC commands
   - Man-in-the-middle on SCADA traffic

3. **Specific SWaT Attack Scenarios:**
   - Single Stage Single Point (SSSP) attacks
   - Single Stage Multi Point (SSMP) attacks
   - Multi Stage Single Point (MSSP) attacks
   - Multi Stage Multi Point (MSMP) attacks

#### c) **BATADAL Dataset Vulnerabilities**

Battle of the Attack Detection Algorithms (BATADAL) focuses on:
- Water distribution network attacks
- Actuator manipulation
- Sensor spoofing
- Cyber-physical attacks on pumps and valves

#### d) **WADI Dataset Coverage**

Water Distribution (WADI) testbed attacks:
- Chemical dosing system attacks
- Water quality manipulation
- Pressure and flow manipulation
- Multi-stage coordinated attacks

#### e) **Attack Feature Distribution**

```python
# Attack type probability in dataset
attack_distribution = {
    'normal': 0.60,      # 60% normal traffic
    'dos': 0.12,         # 12% DoS attacks
    'probe': 0.10,       # 10% reconnaissance
    'r2l': 0.08,         # 8% remote exploits
    'u2r': 0.05,         # 5% privilege escalation
    'modbus_attack': 0.05  # 5% protocol-specific
}
```

#### f) **Feature Engineering for Detection**

The model uses 20+ features extracted from network traffic:
- Packet length and timing
- Protocol-specific flags (TCP, UDP, ICMP, Modbus)
- IP version and header information
- Connection state features
- Statistical aggregations

---

## 7. Dataset Usage and Real-Time Industry Adaptation

### A. How Datasets Are Used in the Lab

#### a) **Dataset Loading Process**

```python
class RealTimeSimulationService:
    def __init__(self, dataset_path="/app/trained_models/balanced_subset.csv"):
        self.dataset_path = dataset_path
        self._load_dataset_from_csv()
    
    def _load_dataset_from_csv(self):
        # Load CSV dataset
        self.dataset = pd.read_csv(self.dataset_path)
        self.total_packets = len(self.dataset)
        
        # Identify feature columns
        metadata_columns = ['timestamp', 'src_ip', 'dst_ip', 
                           'protocol', 'packet_size', 'label', 'category']
        self.feature_columns = [col for col in self.dataset.columns 
                                if col not in metadata_columns]
```

#### b) **Creating Balanced Subsets**

```python
def create_balanced_subset(
    input_path="trained_models/processed_ics_dataset_cleaned.csv",
    output_path="trained_models/balanced_subset.csv",
    samples_per_class=500,
    total_max_samples=5000
):
    """Create balanced dataset for testing"""
    df = pd.read_csv(input_path)
    
    balanced_samples = []
    for class_label in df['label'].unique():
        class_data = df[df['label'] == class_label]
        sample_size = min(len(class_data), samples_per_class)
        sampled_data = class_data.sample(n=sample_size)
        balanced_samples.append(sampled_data)
    
    balanced_df = pd.concat(balanced_samples)
    balanced_df.to_csv(output_path, index=False)
```

#### c) **Real-Time Simulation Flow**

```mermaid
flowchart TD
    A["1️⃣ Dataset Loading"] --> B["2️⃣ Random Packet Sampling"]
    B --> C["3️⃣ Feature Preprocessing"]
    C --> D["4️⃣ ML Model Inference"]
    D --> E["5️⃣ Classification Result"]
    E --> F["6️⃣ WebSocket Broadcast"]
    F --> G["7️⃣ Dashboard Update"]

    A -.-> CSV[("📄 CSV/Database")]
    C -.-> SCALER["📐 StandardScaler"]
    D -.-> MODEL["🧠 ensemble_model.pkl"]
    E -.-> LABELS["🏷️ normal/dos/probe/r2l/u2r"]

    style A fill:#3b82f6,stroke:#60a5fa,color:#fff
    style B fill:#8b5cf6,stroke:#a78bfa,color:#fff
    style C fill:#ec4899,stroke:#f472b6,color:#fff
    style D fill:#f97316,stroke:#fb923c,color:#fff
    style E fill:#eab308,stroke:#facc15,color:#000
    style F fill:#10b981,stroke:#34d399,color:#fff
    style G fill:#06b6d4,stroke:#22d3ee,color:#fff
```

### B. Industry Adaptation Guide

#### a) **Adapting for Your Industry**

To adapt OT-Sentinel for your specific industry:

**Step 1: Collect Your Network Traffic**
```python
# Capture real network traffic using your preferred tool
# Export to CSV with these columns:
columns = [
    'timestamp',
    'src_ip',
    'dst_ip', 
    'protocol',
    'packet_size',
    'label',  # 'normal' or attack type
    'feature_0', 'feature_1', ..., 'feature_19'  # Network features
]
```

**Step 2: Configure Device Mapping**
```python
# Edit devices_data in simple_api.py
devices_data = [
    {
        "id": 1,
        "name": "Your PLC Name",
        "ip_address": "YOUR.IP.ADDRESS",
        "device_type": "PLC",
        "protocol": "Modbus TCP",
        "port": 502,
        "manufacturer": "Your Manufacturer",
        "location": "Your Location"
    },
    # Add your devices...
]
```

**Step 3: Update Process Points**
```python
# Edit industrial_data.py
INDUSTRIAL_CONFIG = {
    "process_name": "Your Process Name",
    "data_source": {
        "host": "YOUR.PLC.IP",
        "port": 502
    },
    "process_points": [
        {
            "name": "Your_Sensor_1",
            "type": "temperature",  # or pressure, flow_rate, etc.
            "unit": "°C",
            "normal_range": [20, 80]
        }
    ]
}
```

**Step 4: Retrain Models (Optional)**
```bash
# If you have labeled attack data, retrain:
python train_model.py --input your_dataset.csv --output trained_models/
```

#### b) **Environment Configuration**

Create `.env` file for your environment:
```bash
# API Configuration
API_HOST=0.0.0.0
API_PORT=8000

# Your Modbus/Industrial Network
MODBUS_HOST=192.168.X.X
MODBUS_PORT=502

# Database (for persistent storage)
DATABASE_URL=postgresql://user:pass@localhost:5432/your_db

# Dataset Path
DATASET_PATH=/path/to/your/dataset.csv
```

#### c) **Docker Deployment**

```bash
# Build and deploy
docker-compose -f docker-compose-gpu.yml up -d

# Access dashboard
# http://localhost:3000
```

#### d) **Supported Industries**

OT-Sentinel can be adapted for:

| Industry | Key Protocols | Typical Devices |
|----------|--------------|-----------------|
| **Water Treatment** | Modbus, DNP3 | PLCs, RTUs, Sensors |
| **Power Generation** | IEC 61850, DNP3 | IEDs, RTUs, HMIs |
| **Manufacturing** | Profinet, EtherNet/IP | PLCs, Robots, VFDs |
| **Oil & Gas** | Modbus, OPC UA | DCS, SCADA, Field Devices |
| **Chemical Processing** | Modbus, Foundation Fieldbus | DCS, Safety PLCs |
| **Building Automation** | BACnet, Modbus | Controllers, Sensors |

#### e) **Expected Data Format**

```csv
packet_length,timestamp,has_ip,has_tcp,has_udp,has_icmp,has_modbus,ip_version,label,category
64,2024-01-15T10:30:00.000Z,1,1,0,0,0,4,normal,normal
128,2024-01-15T10:30:01.000Z,1,0,1,0,1,4,modbus_attack,attack
```

---

## 8. Understanding TenEast Chemical Process & Simulation

### ⚠️ Important Clarification

**TenEast Chemical Process is NOT a publicly available dataset** like SWaT, BATADAL, or WADI. It is a **simulated/demo industrial process** built directly into the OT-Sentinel project for visualization and demonstration purposes.

### A. What is TenEast?

#### Definition
TenEast is a **synthetic chemical processing plant simulation** parsed from a ScadaBR (open-source SCADA software) configuration. It provides the industrial context and visual representation of an OT environment.

#### Purpose
| Component | Role |
|-----------|------|
| **TenEast Simulation** | Provides industrial visualization (valves, tanks, sensors, PLCs) |
| **ML Training Data** | Actual ICS network traffic datasets (attack detection) |

```mermaid
flowchart TB
    subgraph OT_SENTINEL["🛡️ OT-SENTINEL ARCHITECTURE"]
        subgraph TENEAST["🏭 TenEast Process<br/>(SIMULATION)"]
            V["🔧 Valves"]
            F["💧 Flow rates"]
            P["🌡️ Pressure sensors"]
            T["📊 Tank levels"]
            PLC["🎛️ PLCs & HMIs"]
        end

        subgraph ML["🤖 ML Attack Detection<br/>(REAL DATA)"]
            CSV["📄 balanced_subset.csv"]
            TRAFFIC["📡 ICS network traffic"]
            LABELS["🏷️ Attack labels"]
            MODEL["🧠 Trained ensemble model"]
        end

        TENEAST --> DASHBOARD
        ML --> DASHBOARD

        DASHBOARD["📺 UNIFIED DASHBOARD<br/>Real-time Monitoring"]
    end

    style OT_SENTINEL fill:#0f172a,stroke:#3b82f6,color:#fff
    style TENEAST fill:#1e3a5f,stroke:#f97316,color:#fff
    style ML fill:#1e3a5f,stroke:#8b5cf6,color:#fff
    style DASHBOARD fill:#10b981,stroke:#34d399,color:#fff
```

### B. How the Simulation Works

#### Step 1: Industrial Configuration Loading

The simulation loads from `industrial_data.py`:

```python
# TenEast is configured as a Modbus data source
INDUSTRIAL_CONFIG = {
    "process_name": "TenEast Chemical Process",
    "data_source": {
        "name": "TenEast",
        "type": "MODBUS_IP",
        "host": "192.168.95.2",    # Simulated PLC IP (not real)
        "port": 502,               # Standard Modbus port
        "slave_id": 1,
        "update_period": 200       # milliseconds
    }
}
```

#### Step 2: Process Points Generation

Process points (sensors/actuators) are defined and their values are **programmatically generated**:

```python
"process_points": [
    {
        "xid": "DP_909767",
        "name": "AValve",
        "description": "Feed1 Control Valve A",
        "unit": "%",
        "type": "valve_position",
        "normal_range": [0, 100],
        "critical_threshold": 95,
        "modbus_register": 0
    },
    {
        "xid": "DP_399996",
        "name": "ProductValve",
        "description": "Product Output Control Valve",
        "unit": "%",
        "type": "valve_position"
    },
    # ... more process points
]
```

#### Step 3: Real-Time Value Generation

The `IndustrialDataGenerator` class generates realistic sensor values:

```python
class IndustrialDataGenerator:
    """Generate realistic industrial process data"""
    
    def get_current_data(self):
        """Generate current values for all process points"""
        current_data = []
        
        for point in INDUSTRIAL_CONFIG["process_points"]:
            # Generate value based on point type
            base_value = self._get_base_value(point)
            variation = self.get_variation(point["type"])
            
            # Add realistic noise/variation
            value = base_value + random.uniform(-variation, variation)
            
            current_data.append({
                "id": point["xid"],
                "name": point["name"],
                "value": value,
                "unit": point["unit"],
                "timestamp": datetime.utcnow().isoformat(),
                "alarm_status": self._check_alarm(point, value)
            })
        
        return current_data
    
    def get_variation(self, point_type):
        """Get typical variation for different sensor types"""
        variations = {
            "valve_position": 2.0,    # ±2% variation
            "flow_rate": 15.0,        # ±15 GPM variation
            "pressure": 25.0,         # ±25 PSI variation
            "level": 1.5,             # ±1.5% variation
            "composition": 3.0        # ±3% variation
        }
        return variations.get(point_type, 1.0)
```

#### Step 4: Alarm Generation from Process Values

```python
def _check_alarm(self, point, value):
    """Check if value triggers alarm"""
    if value >= point.get("critical_threshold", 100):
        return "critical"
    elif value >= point.get("warning_threshold", 90):
        return "warning"
    return "normal"

def generate_alerts(self):
    """Generate process alerts based on current conditions"""
    alerts = []
    current_data = self.get_current_data()
    
    for point_data in current_data:
        if point_data["alarm_status"] == "critical":
            alerts.append({
                "id": f"ALERT_{point_data['id']}_{timestamp}",
                "alert_type": f"Critical {point_data['type']}",
                "severity": "critical",
                "description": f"{point_data['description']} exceeded threshold: {point_data['value']}"
            })
    
    return alerts
```

### C. Simulation vs Real Detection - Key Differences

| Aspect | TenEast Simulation | ML Attack Detection |
|--------|-------------------|---------------------|
| **Data Source** | Programmatically generated | CSV dataset / Real network traffic |
| **Purpose** | Visualization & Demo | Actual threat detection |
| **Values** | Random within normal ranges | Extracted from packets |
| **Alerts** | Threshold-based | ML classification-based |
| **Accuracy** | N/A (synthetic) | 99.1% accuracy |

### D. How They Work Together

```mermaid
flowchart TB
    subgraph FLOW["🔄 SIMULATION FLOW"]
        subgraph VIZ["1️⃣ VISUALIZATION LAYER (TenEast)"]
            V1["🔧 IndustrialDataGenerator.get_current_data()"]
            V2["📊 Generate valve positions, flow rates, pressures"]
            V3["⚠️ Check thresholds → Generate process alarms"]
            V4["🖥️ Display on Dashboard"]
            V1 --> V2 --> V3 --> V4
        end

        subgraph SEC["2️⃣ SECURITY LAYER (ML Detection)"]
            S1["📄 Load balanced_subset.csv"]
            S2["📦 RealTimeSimulationService.get_next_packet()"]
            S3["🔬 Preprocess features → ML Model inference"]
            S4["🏷️ Classify: normal/dos/probe/r2l/u2r"]
            S5["📡 Broadcast via WebSocket"]
            S1 --> S2 --> S3 --> S4 --> S5
        end

        subgraph DASH["3️⃣ UNIFIED DASHBOARD"]
            D1["📈 Process Overview"]
            D2["🗺️ Network Topology"]
            D3["🚨 Security Alerts"]
            D4["📅 Attack Timeline"]
            D5["📊 Real-time Traffic"]
        end

        V4 --> DASH
        S5 --> DASH
    end

    style FLOW fill:#0f172a,stroke:#3b82f6,color:#fff
    style VIZ fill:#1e3a5f,stroke:#f97316,color:#fff
    style SEC fill:#1e3a5f,stroke:#8b5cf6,color:#fff
    style DASH fill:#1e3a5f,stroke:#10b981,color:#fff
```

### E. ScadaBR Configuration Origin

TenEast originates from a ScadaBR JSON export:

```python
# Original ScadaBR configuration structure
SCADABR_CONFIG = {
    "dataSources": [
        {
            "name": "TenEast",
            "enabled": True,
            "host": "192.168.95.2",
            "port": 502,
            "updatePeriods": 200
        }
    ],
    "dataPoints": [
        {
            "xid": "DP_909767",
            "name": "AValve",
            "deviceName": "TenEast",
            "engineeringUnits": "%",
            "enabled": True,
            "pointLocator": {
                "range": "HOLDING_REGISTER",
                "modbusDataType": "TWO_BYTE_INT_SIGNED",
                "offset": 0
            }
        }
        # ... more data points
    ],
    "watchLists": [
        {
            "name": "TenEastHistory",
            "dataPoints": ["DP_816839", "DP_159283", ...]
        }
    ]
}
```

### F. Why This Design?

#### Benefits of Separation:

1. **Realistic Visualization** - TenEast provides context that makes the dashboard look like a real ICS monitoring system

2. **Accurate Detection** - ML models are trained on actual ICS attack datasets, not synthetic data

3. **Flexibility** - You can replace TenEast with your own industrial process configuration while keeping the ML detection intact

4. **Demo-Ready** - Works out-of-the-box without needing a real PLC or industrial network

### G. Customizing TenEast for Your Industry

To replace TenEast with your own process:

```python
# Edit industrial_data.py
INDUSTRIAL_CONFIG = {
    "process_name": "Your Chemical Plant Name",
    "data_source": {
        "name": "YourProcess",
        "type": "MODBUS_IP",
        "host": "YOUR.PLC.IP.ADDRESS",  # Real or simulated
        "port": 502
    },
    "process_points": [
        {
            "name": "ReactorTemp",
            "description": "Main Reactor Temperature",
            "unit": "°C",
            "type": "temperature",
            "normal_range": [150, 250],
            "critical_threshold": 280
        },
        {
            "name": "CoolingFlow",
            "description": "Cooling Water Flow Rate",
            "unit": "L/min",
            "type": "flow_rate",
            "normal_range": [100, 500]
        }
        # Add your process points...
    ]
}
```

---

## Additional Resources

### Model Performance Metrics

| Metric | OT-Sentinel | Industry Average |
|--------|-------------|------------------|
| **Accuracy** | 99.1% | 85-92% |
| **F1-Score** | 98.9% | 80-88% |
| **Precision** | 99.2% | 82-90% |
| **Recall** | 98.7% | 78-86% |
| **False Positive Rate** | 0.8% | 5-12% |
| **Inference Latency** | <50ms | Variable |
| **Throughput** | 1000+ packets/sec | Variable |

### Quick Start Commands

```bash
# Clone repository
git clone https://github.com/BipinRajC/OT-sentinel.git
cd OT-sentinel

# Deploy with Docker (GPU)
docker-compose -f docker-compose-gpu.yml up -d

# Deploy with Docker (CPU only)
docker-compose -f docker-compose.yml up -d

# Access Dashboard
# Open browser: http://localhost:3000

# Check logs
docker-compose logs backend
docker-compose logs frontend
```

### Support & Documentation

- **Main README**: [README.md](https://github.com/BipinRajC/OT-Sentinel/blob/main/README.md)
- **Setup Guide**: [SETUP.md](https://github.com/BipinRajC/OT-Sentinel/blob/main/SETUP.md)
- **Real-time Dashboard Docs**: [README-realtime-dashboard.md](https://github.com/BipinRajC/OT-Sentinel/blob/main/README-realtime-dashboard.md)

---

*Document generated for OT-Sentinel v1.0.0 | Last Updated: January 2026*
