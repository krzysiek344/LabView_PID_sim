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

* ## Embedded Application (Zephyr RTOS)

The embedded application runs on **STM32 Nucleo F446RE** microcontroller and simulates a DC motor with encoder for PID controller testing.

### Architecture

The application is built on **Zephyr RTOS** and consists of three main threads:

1. **Simulation Thread** (Priority 5)
   - Updates motor physics model every 10ms
   - Calculates position based on PWM input and motor dynamics

2. **Position TX Thread** (Priority 6)
   - Sends current position via UART every 100ms
   - Format: `POS:<position>\n` (e.g., `POS:400.0\n`)

3. **Main Thread** (Priority 7)
   - Receives PWM values from LabVIEW via UART interrupt handler
   - Updates motor simulator with new PWM values
   - Handles RX timeout for incomplete messages

### Motor Simulation Model

The simulator implements a physics-based model with inertia:

- **Drive Force:** `F_drive = PWM × gain`
- **Friction Force:** `F_friction = velocity × friction`
- **Acceleration:** `a = (F_drive - F_friction) / mass`
- **Velocity:** Updated based on acceleration
- **Position:** Updated based on velocity

**Key Parameters** (configurable in `src/motor_simulator.c`):
- `gain`: PWM to force conversion (default: `0.009804f`)
- `friction`: Friction coefficient (default: `0.5f`)
- `mass`: Motor inertia (default: `10.0f`) - higher = more inertia
- `max_velocity`: Maximum velocity limit (default: `5.0f`)
- Initial position: `400.0` (range: 0-1000)

### Hardware Configuration

- **Board:** STM32 Nucleo F446RE
- **UART:** UART4 (PC10 TX, PC11 RX) @ 115200 baud
- **Communication:** Interrupt-driven UART RX, polling TX
