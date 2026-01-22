# LabVIEW PID Position Control via UART

This project implements a closed-loop control system using **LabVIEW** as the primary PID controller. The PC communicates with an external actuator (microcontroller) via a serial interface,
receiving real-time position data and sending back control signals (PWM).

## System Overview

The main application (`Main_PID.vi`) runs on the PC and acts as a digital controller.
1.  **Data Reception:** LabVIEW listens on the COM port for the object's current position.
2.  **Processing:** It calculates the error (difference between the *Setpoint* and *Current Position*) and processes it through the PID algorithm.
3.  **Control:** It sends the calculated control value (PWM) back to the microcontroller.

## Requirements

* **Software:**
    * NI LabVIEW (requires the **NI-VISA** package for serial communication).
* **Hardware:**
    * Microcontroller (e.g., Arduino, STM32) connected via USB/UART.
    * Actuator system (e.g., DC motor with an encoder).

## Communication Protocol

Communication is text-based (ASCII). The termination character for every command is **Line Feed** (`\n`, ASCII 10).

### 1. LabVIEW -> Microcontroller (Control Signal)
LabVIEW sends an integer value within the range of **-255 to 255**.

* **Positive Value (0 to 255):** Move Right / Forward.
* **Negative Value (-255 to 0):** Move Left / Backward.
* **0:** Stop.
* **Format:** `[value]\n` (e.g., `150\n`, `-200\n`).

> **Reset Procedure:** When the controller is stopped or reset, LabVIEW sends the sequence `0\n` **five times** (with a short delay) to ensure the motor stops safely.

### 2. Microcontroller -> LabVIEW (Feedback)
LabVIEW expects a data frame containing the current position.

* **Format:** `POS:[float_value]\n`
* **Example:** `POS:125.50\n`
