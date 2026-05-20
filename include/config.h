#ifndef CONFIG_H
#define CONFIG_H

// Lcd I2C address
#define LCD_ADDR 0x27

// System state variables
#define IDLE_STATE 0
#define POURING_STATE 1

// Pin definitions for the ATmega328P (PD2 = IR Sensor, PD3 = Start Button, PD5-7 = Relays)
#define BIT_IR      2
#define BIT_BTN     3
#define BIT_PUMP1   5
#define BIT_PUMP2   6
#define BIT_PUMP3   7

// Calibration constants for converting ADC readings to milliliters
#define MAX_ADC_RAW         970 // Max ADC value corresponding to 125 ml (calibrated experimentally)
#define MAX_VOL_ML          125 // Maximum volume capped for each liquid
#define TIME_MS_PER_ML      40  // How many milliseconds it takes to pour a single milliliter
#define ADC_FILTER          4   // The threshold for filtering noise from the potentiometers

// --- SETĂRI TIMPI (Milisecunde) ---
#define LCD_REFRESH_RATE    100  // The refresh rate of the screen
#define IR_PANIC_THRESHOLD  150  // The time required to confirm an error (missing glass)
#define ERROR_DISPLAY_TIME  3000 // How long the error message is displayed
#define SUCCESS_DISPLAY_TIME 2000 // How long the success message is displayed

// --- SETĂRI COMUNICAȚIE ---
#define I2C_BIT_RATE 72 // Setting for 100 kHz SCL at 16MHz

#endif /* CONFIG_H */