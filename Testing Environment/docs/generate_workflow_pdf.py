#!/usr/bin/env python3
"""Generate SYSTEM_WORKFLOW.pdf for the SDI-12 sensor project."""

from fpdf import FPDF
from pathlib import Path

OUT = Path(__file__).resolve().parent / "SYSTEM_WORKFLOW.pdf"


class WorkflowPDF(FPDF):
    def footer(self):
        self.set_y(-15)
        self.set_font("Helvetica", "I", 8)
        self.cell(0, 10, f"Page {self.page_no()}/{{nb}}", align="C")


def section(pdf: FPDF, title: str):
    pdf.ln(4)
    pdf.set_font("Helvetica", "B", 12)
    pdf.set_fill_color(230, 240, 255)
    pdf.cell(0, 8, title, ln=True, fill=True)
    pdf.set_font("Helvetica", "", 10)


def bullet(pdf: FPDF, text: str):
    pdf.set_x(pdf.l_margin + 2)
    pdf.multi_cell(pdf.epw - 2, 5, f"- {text}")


def code(pdf: FPDF, text: str):
    pdf.set_font("Courier", "", 9)
    pdf.set_fill_color(245, 245, 245)
    for line in text.strip().split("\n"):
        pdf.cell(0, 5, "  " + line, ln=True, fill=True)
    pdf.set_font("Helvetica", "", 10)


def build_pdf():
    pdf = WorkflowPDF()
    pdf.alias_nb_pages()
    pdf.set_auto_page_break(auto=True, margin=18)
    pdf.add_page()

    pdf.set_font("Helvetica", "B", 18)
    pdf.cell(0, 12, "SDI-12 Sensor Station - System Workflow", ln=True)
    pdf.set_font("Helvetica", "", 11)
    pdf.multi_cell(
        0,
        6,
        "Board: Arduino Zero / MKR / SAMD21 (TC4 hardware timer). "
        "This document explains how main.ino and all modules work together.",
    )

    section(pdf, "0. Where to START reading the code")
    pdf.set_font("Helvetica", "B", 10)
    pdf.multi_cell(pdf.epw, 5, "Follow this order the first time you open the project:")
    pdf.ln(1)
    code(
        pdf,
        """
  START --> main.ino (setup + loop order)
            |
            v
         HardwareConfig.h (pins + all timing constants)
            |
            v
         SensorReading.h / .cpp (BME280 + BH1750, readSensors)
            |
            +-------- choose your branch --------+
            |              |          |         |
            v              v          v         v
      Dashboard.cpp   SdCard.cpp  SensorSampler  sdi12.cpp
      (TFT real-time)      |       .cpp (2s TC4)  (SDI-12)
                           |
                      +----+----+
                      v         v
                 DataLogger   AvgDataLogger
                 datalog.csv  avg_datalog.csv
        """,
    )
    pdf.ln(1)
    steps = [
        ("1", "main.ino", "Only entry point - what runs and in what order"),
        ("2", "HardwareConfig.h", "Pins and intervals (2s, 60s, 1s)"),
        ("3", "SensorReading.cpp", "All sensors - every module uses this"),
        ("4a", "Dashboard.cpp", "TFT display (real-time reads)"),
        ("4b", "SdCard + DataLogger", "datalog.csv + buttons"),
        ("4c", "SensorSampler + AvgDataLogger", "2s averages + avg_datalog.csv"),
        ("4d", "sdi12.cpp", "SDI-12 commands M, D1, D2, R0"),
    ]
    pdf.set_font("Helvetica", "B", 9)
    pdf.cell(10, 6, "Step", border=1)
    pdf.cell(50, 6, "File", border=1)
    pdf.cell(0, 6, "Why", border=1, ln=True)
    pdf.set_font("Helvetica", "", 9)
    for s, f, w in steps:
        pdf.cell(10, 6, s, border=1)
        pdf.cell(50, 6, f, border=1)
        pdf.cell(0, 6, w, border=1, ln=True)
    pdf.ln(2)
    pdf.set_font("Helvetica", "I", 9)
    pdf.multi_cell(
        pdf.epw,
        5,
        "Rule: Always read main.ino -> HardwareConfig.h -> SensorReading first. "
        "Then open only the branch you need.",
    )

    section(pdf, "1. Big picture (two separate sensor paths)")
    bullet(pdf, "REAL-TIME path: instant I2C reads for TFT, SDI-12, and datalog.csv")
    bullet(pdf, "AVERAGED path: TC4 timer every 2 s -> avg_datalog.csv only")
    pdf.ln(2)
    code(
        pdf,
        """
                    +------------------+
                    |   main.ino       |
                    |  setup() / loop()|
                    +--------+---------+
                             |
         +-------------------+-------------------+
         |                                       |
         v                                       v
  REAL-TIME (on demand)                  AVERAGED (timer-driven)
  - Dashboard: readSensors()             - SensorSampler (TC4 ISR)
  - SDI-12: readSensors() on M/D1/D2/R0  - AvgDataLogger -> avg_datalog.csv
  - DataLogger: readSensors()            - Every 2 seconds
    -> datalog.csv every 60 s
        """,
    )

    section(pdf, "2. Module map (what each file does)")
    rows = [
        ("main.ino", "Entry point. Calls all init and loop handlers in order."),
        ("HardwareConfig.h", "Pins, intervals: dashboard 1s, datalog 60s, sample 2s."),
        ("SensorReading.cpp", "BME280 + BH1750 I2C. readSensors(), getSensorData()."),
        ("SensorSampler.cpp", "TC4 ISR (10ms base, 2s sample). Builds 2s averages."),
        ("SdCard.cpp", "One SD card + RTC shared by both loggers."),
        ("DataLogger.cpp", "datalog.csv - periodic (60s) + manual button."),
        ("AvgDataLogger.cpp", "avg_datalog.csv - one row per 2s average."),
        ("Dashboard.cpp", "TFT display - real-time readSensors(), NOT averages."),
        ("sdi12.cpp", "SDI-12 slave on Serial1. Uses getSensorData() after read."),
    ]
    pdf.set_font("Helvetica", "B", 9)
    pdf.cell(45, 6, "File", border=1)
    pdf.cell(0, 6, "Role", border=1, ln=True)
    pdf.set_font("Helvetica", "", 9)
    for f, r in rows:
        pdf.cell(45, 6, f, border=1)
        pdf.cell(0, 6, r, border=1, ln=True)

    section(pdf, "3. setup() order (runs once at power-on)")
    code(
        pdf,
        """
  Serial.begin(9600)
  sensorsInit()          // Wire + BME280 + BH1750
  sdCardInit()           // RTC + SD card (shared)
  sensorSamplerInit()    // Start TC4 timer (SAMD21)
  dataLoggerInit()       // Buttons + datalog.csv header
  avgDataLoggerInit()    // avg_datalog.csv header
  dashboardInit()        // TFT labels
  sdi12Init('0', pin 8)  // SDI-12 bus
  readSensors()          // First reading
        """,
    )

    section(pdf, "4. loop() order (runs forever)")
    code(
        pdf,
        """
  1. sensorSamplerService()  // FIRST: process 2s timer flags, I2C for averages
  2. sdi12Handle()           // SDI-12 commands + R0 stream (1s)
  3. avgDataLoggerUpdate()   // Write avg_datalog.csv when new 2s average ready
  4. dataLoggerUpdate()      // 60s log + manual/clear buttons
  5. dashboardUpdate()       // TFT refresh every 1s, real-time readSensors()
        """,
    )

    pdf.add_page()
    section(pdf, "5. Averaged path detail (SensorSampler)")
    bullet(pdf, "TC4 hardware interrupt fires every ~10 ms (base tick).")
    bullet(pdf, "ISR counts 200 ticks -> one sample request every 2000 ms (2 s).")
    bullet(pdf, "ISR only sets g_sampleTicksPending (NO I2C inside interrupt).")
    bullet(pdf, "sensorSamplerService() in loop reads sensors and accumulates.")
    bullet(pdf, "After 1 sample in window -> finalizeAverageWindow().")
    bullet(pdf, "AvgDataLogger writes row and calls sensorSamplerAcknowledgeAverage().")
    code(
        pdf,
        """
  TC4 ISR (every 10ms)     loop: sensorSamplerService()
       |                              |
       +-- tick divider (x200)        +-- readSensors() if pending
       +-- g_sampleTicksPending++     +-- finalize -> newAverageAvailable
                                              |
                                              v
                                    avgDataLoggerUpdate()
                                              |
                                              v
                                    avg_datalog.csv (every ~2s)
        """,
    )

    section(pdf, "6. Real-time path detail (everything else)")
    bullet(pdf, "Dashboard: every 1000 ms calls readSensors() then getSensorData().")
    bullet(pdf, "DataLogger: every 60000 ms OR button pin 2 -> datalog.csv row.")
    bullet(pdf, "SDI-12: host sends command ending with ! -> readSensors() -> reply.")
    bullet(pdf, "SDI-12 R0 stream: every 1000 ms while streaming -> readSensors().")
    bullet(pdf, "Clear SD button (pin 3): deletes datalog.csv only (not avg file).")

    section(pdf, "7. CSV files (do not mix them up)")
    pdf.set_font("Helvetica", "B", 9)
    pdf.cell(40, 6, "File", border=1)
    pdf.cell(25, 6, "Interval", border=1)
    pdf.cell(0, 6, "Data type", border=1, ln=True)
    pdf.set_font("Helvetica", "", 9)
    pdf.cell(40, 6, "datalog.csv", border=1)
    pdf.cell(25, 6, "60 s + manual", border=1)
    pdf.cell(0, 6, "Instant snapshot at log time", border=1, ln=True)
    pdf.cell(40, 6, "avg_datalog.csv", border=1)
    pdf.cell(25, 6, "~2 s", border=1)
    pdf.cell(0, 6, "Hardware-timed average (1 sample/window)", border=1, ln=True)

    section(pdf, "8. SDI-12 commands (address 0 by default)")
    cmds = [
        ("? or 0!", "Respond with sensor address"),
        ("0M!", "Measure: returns count of parameters"),
        ("0D1!", "Send BME: temp, humidity, pressure"),
        ("0D2!", "Send BH1750: lux"),
        ("0R0!", "Send all + start continuous stream every 1 s"),
        ("0An!", "Change address to n"),
    ]
    for c, d in cmds:
        bullet(pdf, f"{c}  ->  {d}")

    section(pdf, "9. Timing constants (HardwareConfig.h)")
    timings = [
        ("kSensorSampleMs", "2000 ms", "Average path: time between I2C samples"),
        ("kSensorAverageMs", "2000 ms", "Length of one average window"),
        ("kTc4BasePeriodMs", "10 ms", "TC4 hardware tick (divider -> 2 s)"),
        ("kDashboardRefreshMs", "1000 ms", "TFT update rate"),
        ("kLogIntervalMs", "60000 ms", "datalog.csv auto-log interval"),
        ("R0_STREAM_INTERVAL_MS", "1000 ms", "SDI-12 R0 stream (in sdi12.cpp)"),
    ]
    for name, val, note in timings:
        bullet(pdf, f"{name} = {val}  ({note})")

    section(pdf, "10. Common confusion - quick answers")
    qa = [
        ("Why two sensor reads?", "Sampler and dashboard/logger both call readSensors() for different purposes."),
        ("Does TFT show averages?", "No. Dashboard always does a fresh real-time read."),
        ("Does SDI-12 use averages?", "No. It uses getSensorData() after readSensors()."),
        ("What does TC4 do?", "Only schedules 2 s ticks. I2C runs in loop, not in ISR."),
        ("Non-SAMD board?", "SensorSampler falls back to millis() - less precise timing."),
    ]
    pdf.set_x(pdf.l_margin)
    for q, a in qa:
        pdf.set_font("Helvetica", "B", 9)
        pdf.multi_cell(pdf.epw, 5, f"Q: {q}")
        pdf.set_font("Helvetica", "", 9)
        pdf.multi_cell(pdf.epw, 5, f"A: {a}")
        pdf.ln(1)

    pdf.ln(4)
    pdf.set_font("Helvetica", "I", 9)
    pdf.cell(0, 6, "Generated for: Testing Environment/main/", ln=True)

    pdf.output(OUT)
    return OUT


if __name__ == "__main__":
    path = build_pdf()
    print(f"Wrote {path}")
