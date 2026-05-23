# Code reading flowchart

**Start here:** `main/main.ino`

Open this file in GitHub, VS Code, or any Mermaid preview to see the interactive chart.

---

## Main reading path

```mermaid
flowchart TD
  START(["YOU ARE HERE<br/>main.ino"])
  M1["Read setup()<br/>lines 12-25"]
  M2["Read loop()<br/>lines 28-35"]
  H["HardwareConfig.h<br/>pins + kSensorSampleMs etc."]
  SR["SensorReading.h<br/>SensorData struct"]
  SRC["SensorReading.cpp<br/>readSensors() implementation"]

  START --> M1
  M1 --> M2
  M2 --> H
  H --> SR
  SR --> SRC
  SRC --> PICK{"What feature<br/>next?"}

  PICK -->|Screen| D1["Dashboard.cpp<br/>dashboardUpdate()"]
  PICK -->|SD periodic log| D2["SdCard.cpp<br/>then DataLogger.cpp"]
  PICK -->|SD average log| D3["SensorSampler.cpp<br/>then AvgDataLogger.cpp"]
  PICK -->|SDI-12 bus| D4["sdi12.cpp<br/>parseCommand() + sdi12Handle()"]

  D1 --> END([You understand that path])
  D2 --> END
  D3 --> END
  D4 --> END
```

---

## How `loop()` connects files (runtime flow)

Read this **after** step 3 (`SensorReading`).

```mermaid
flowchart LR
  L[loop in main.ino]

  L --> S1[sensorSamplerService<br/>SensorSampler.cpp]
  L --> S2[sdi12Handle<br/>sdi12.cpp]
  L --> S3[avgDataLoggerUpdate<br/>AvgDataLogger.cpp]
  L --> S4[dataLoggerUpdate<br/>DataLogger.cpp]
  L --> S5[dashboardUpdate<br/>Dashboard.cpp]

  S1 --> R[readSensors<br/>SensorReading.cpp]
  S2 --> R
  S3 --> R
  S4 --> R
  S5 --> R

  S3 --> SD[(avg_datalog.csv)]
  S4 --> SD2[(datalog.csv)]
  S1 --> SD
  S4 --> SD2
```

---

## Quick reference

| Order | File | Look for |
|-------|------|----------|
| 1 | `main.ino` | `setup()`, `loop()` |
| 2 | `HardwareConfig.h` | `kSensorSampleMs`, pins |
| 3 | `SensorReading.cpp` | `readSensors()`, `getSensorData()` |
| 4 | *(pick one)* | See branches above |

**Do not start with** `SensorSampler.cpp` or `AvgDataLogger.cpp` — they only make sense after `main.ino` and `SensorReading.cpp`.

---

See also: `SYSTEM_WORKFLOW.md` and `SYSTEM_WORKFLOW.pdf`
