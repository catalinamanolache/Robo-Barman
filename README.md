# Robo-Barman: Automated Cocktail Dispenser

## Overview
**Robo-Barman** is an asynchronous, bare-metal embedded C project built for the ATmega328P microcontroller. It serves as an automated liquid dispenser that mixes three different liquids into custom cocktails. 

The system relies on a *purely analog interface (three potentiometers)* for volume selection, provides *real-time optical feedback via an I2C LCD*,
and implements *strict safety mechanisms using an infrared optical sensor* to detect the presence of the glass. 

The software is built around a 
non-blocking **Finite State Machine**, hardware interrupts and custom digital signal filters to handle electromagnetic interference from the 
mechanical pumps.

## Hardware
* **MCU:** ATmega328P
* **Inputs:** 3x 10kΩ Linear Potentiometers, 1x Arcade Start Button, 1x IR Obstacle Sensor (Glass detection)
* **Outputs:** 3x DC Submersible Pumps (3-5V), 1x 4-Channel 5V Relay Module, 1x 1602 LCD Display with I2C module
* **Power:** External 5V/2A power supply for pumps (sharing common GND with MCU)

## Project Structure

### 📁 `include/` (Headers & Configurations)
* **`config.h`**: It defines hardware pinouts, maximum volume ceilings (125 ml), pump flow-rate calibration (`TIME_MS_PER_ML`), LCD refresh rates 
and noise filter thresholds.
* **`adc.h`**, **`i2c.h`**, **`lcd.h`**, **`timer.h`**: Header files exposing the public functions of each hardware module.

### 📁 `src/` (Implementations)
* **`main.c`**: The core of the application. It initializes all peripherals, configures GPIOs, and runs the infinite loop containing the FSM 
(State 0: Idle, State 1: Pouring).
* **`adc.c`**: Handles the Analog-to-Digital Converter. Initializes the ADC with a 128 prescaler and implements a polling-based single-conversion 
read function `ADC_Read()` to fetch potentiometer values.
* **`i2c.c`**: Bare-metal implementation of the TWI protocol. Handles START, STOP and write conditions required to communicate with the LCD at 100 
kHz.
* **`lcd.c`**: HD44780 LCD driver over I2C. Handles 4-bit nibble formatting, cursor positioning and screen clearing.
* **`timer.c`**: Configures Timer1 in CTC mode to trigger an interrupt exactly every 1 millisecond. Contains the `system_ticks` global counter and
the crucial non-blocking delay function `Wait_And_Check_IR()`.

## Software Architecture

The system is designed to be **100% non-blocking** during critical operations. It avoids standard blocking functions like `_delay_ms()` to ensure the microcontroller remains responsive to safety sensors at all times.

### 1. The Core Algorithm (Finite State Machine)
The logic is divided into two mutually exclusive states:
* **STATE 0 (IDLE):**
  * Continuously reads the 3 potentiometers.
  * Converts the raw 10-bit ADC data into milliliters (0-125 ml).
  * Updates the LCD screen asynchronously every 100ms using the `system_ticks` tracker.
  * Validates user input (prevents starting if all volumes are 0).
  * Transitions to State 1 only if the IR sensor confirms the glass is present AND the start button is pressed.
* **STATE 1 (POURING):**
  * Calculates the exact required running time for each pump (e.g., 40ms per 1ml).
  * Sequentially triggers the relays (Pump 1 -> Pump 2 -> Pump 3).
  * During pumping, the LCD is disabled/cleared to prevent I2C bus corruption from the pumps' EMI.
  * If the IR sensor detects the glass being removed mid-pour, the FSM instantly kills all relays, triggers an Emergency Stop sequence, and resets.
  * On success, waits for the user to physically remove the finished cocktail before returning to State 0.

### 2. Advanced Concepts 

* **ADC Jitter Elimination:** Analog components are susceptible to electrical noise, causing the LCD values to flicker rapidly. The algorithm
implements a software *deadband*: the current ADC reading is only updated if the absolute difference between the new reading and the last stable
reading is strictly greater than 4 units (`ADC_FILTER`).
  
* **Multitasking via Timer1:**
  By utilizing a hardware timer interrupt (`TIMER1_COMPA_vect`) that fires every 1ms, the global variable `system_ticks` acts as an absolute timestamp. This allows the system to execute tasks like rate-limiting the LCD refreshes while simultaneously polling sensors at maximum CPU speed.

* **EMI Noise Filtering:**
  When DC motors start, they generate massive electromagnetic spikes that can cause false triggers on the IR sensor pin. Instead of trusting a single digital read, the `Wait_And_Check_IR()` function monitors the pin every single millisecond. It increments an error counter if the glass is missing and decrements it if the glass is present. An emergency stop is only triggered if the glass is completely missing for 150 consecutive milliseconds (`IR_PANIC_THRESHOLD`), effectively filtering out all false electrical spikes.
