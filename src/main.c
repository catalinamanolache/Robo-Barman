#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "config.h"
#include "i2c.h"
#include "lcd.h"
#include "adc.h"
#include "timer.h"

void init(void) {
    // Initialize communication protocols and peripherals
    I2C_Init(); 
    LCD_Init(); 
    ADC_Init();
    
    // Initialize the background timer and enable global interrupts
    Timer1_Init_Systick();
    sei(); 

    // Configure Input Pins (PD2 = IR Sensor, PD3 = Start Button)
    DDRD &= ~(1 << BIT_IR); 
    DDRD &= ~(1 << BIT_BTN);
    
    // Enable internal Pull-Up resistors for stable input signals
    PORTD |= (1 << BIT_BTN); 
    PORTD |= (1 << BIT_IR);

    // Configure Output Pins for the 3 Relays (PD5, PD6, PD7)
    DDRD |= (1 << BIT_PUMP1) | (1 << BIT_PUMP2) | (1 << BIT_PUMP3);
    
    // Ensure all relays are turned OFF initially
    PORTD &= ~((1 << BIT_PUMP1) | (1 << BIT_PUMP2) | (1 << BIT_PUMP3));
}


void update_volumes(uint8_t *vol1, uint8_t *vol2, uint8_t *vol3) {
    static uint16_t last_raw_pot1 = 0;
    static uint16_t last_raw_pot2 = 0;
    static uint16_t last_raw_pot3 = 0;

    // Read raw values from the 3 potentiometers
    uint16_t current_raw_pot1 = ADC_Read(0);
    uint16_t current_raw_pot2 = ADC_Read(1);
    uint16_t current_raw_pot3 = ADC_Read(2);

    // If the change is significant, update the last known value. Otherwise, ignore it to filter out noise.
    uint16_t diff1 = (current_raw_pot1 > last_raw_pot1) ? (current_raw_pot1 - last_raw_pot1) : (last_raw_pot1 - current_raw_pot1);
    if (diff1 > ADC_FILTER) last_raw_pot1 = current_raw_pot1;

    uint16_t diff2 = (current_raw_pot2 > last_raw_pot2) ? (current_raw_pot2 - last_raw_pot2) : (last_raw_pot2 - current_raw_pot2);
    if (diff2 > ADC_FILTER) last_raw_pot2 = current_raw_pot2;

    uint16_t diff3 = (current_raw_pot3 > last_raw_pot3) ? (current_raw_pot3 - last_raw_pot3) : (last_raw_pot3 - current_raw_pot3);
    if (diff3 > ADC_FILTER) last_raw_pot3 = current_raw_pot3;

    // Convert the raw 10-bit values (0-970) into milliliters (0-125 ml)
    *vol1 = (uint32_t)last_raw_pot1 * MAX_VOL_ML / MAX_ADC_RAW;
    *vol2 = (uint32_t)last_raw_pot2 * MAX_VOL_ML / MAX_ADC_RAW;
    *vol3 = (uint32_t)last_raw_pot3 * MAX_VOL_ML / MAX_ADC_RAW;

    // Apply safety ceilings to ensure values do not exceed 125 ml
    if(*vol1 > MAX_VOL_ML) *vol1 = MAX_VOL_ML; 
    if(*vol2 > MAX_VOL_ML) *vol2 = MAX_VOL_ML; 
    if(*vol3 > MAX_VOL_ML) *vol3 = MAX_VOL_ML;
}


void process_idle_state(uint8_t vol1, uint8_t vol2, uint8_t vol3, uint8_t *system_state) {
    // Tracks the last time the LCD was updated
    static uint32_t last_lcd_update = 0; 
    
    // String buffer for LCD text formatting
    char lcd_buffer[17]; 

    // Read physical sensor states
    uint8_t ir_state = (PIND & (1 << BIT_IR)) ? 0 : 1; 
    uint8_t button_pressed = (PIND & (1 << BIT_BTN)) ? 0 : 1;

    // --- STATE 0: IDLE / WAITING FOR INPUT ---
    // Refresh the LCD screen only once every 100ms to avoid flickering
    if ((system_ticks - last_lcd_update) > LCD_REFRESH_RATE) {
        last_lcd_update = system_ticks;
        
        // Format the volumes with spacing (e.g., "  0/ 25/125 ml ")
        sprintf(lcd_buffer, "%3d/%3d/%3d ml ", vol1, vol2, vol3);
        LCD_SetCursor(0, 0); 
        LCD_String(lcd_buffer);

        // Validation logic for user input
        if (vol1 == 0 && vol2 == 0 && vol3 == 0) {
            LCD_SetCursor(1, 0); 
            LCD_String("Eroare: Volum 0!");
        }
        else if (ir_state == 1 && button_pressed == 1) {
            *system_state = POURING_STATE; // Transition to pouring state
            LCD_Cmd(0x01); // Clear screen
        } 
        else if (ir_state == 1) {
            LCD_SetCursor(1, 0); 
            LCD_String("Pahar OK. Apasa!");
        } 
        else {
            LCD_SetCursor(1, 0); 
            LCD_String("Pune paharul... ");
        }
    }
}


void process_pouring_state(uint8_t vol1, uint8_t vol2, uint8_t vol3, uint8_t *system_state) {
    // --- STATE 1: SEQUENTIAL POURING ---
    // Calculate pumping times (40 milliseconds per 1 ml)
    uint16_t time1_ms = vol1 * TIME_MS_PER_ML;
    uint16_t time2_ms = vol2 * TIME_MS_PER_ML;
    uint16_t time3_ms = vol3 * TIME_MS_PER_ML;

    // Flag to track critical interruptions
    uint8_t glass_error = 0;

    LCD_Cmd(0x01);
    
    // --- PUMP 1 BLOCK ---
    if (time1_ms > 0 && glass_error == 0) {
        // Engage Relay 1
        PORTD |= (1 << BIT_PUMP1);    
        
        // Wait and monitor for glass removal
        if (Wait_And_Check_IR(time1_ms) == 0) { 
            glass_error = 1; 
        }
        
        // Disengage Relay 1
        PORTD &= ~(1 << BIT_PUMP1);
    }

    // --- PUMP 2 BLOCK ---
    if (time2_ms > 0 && glass_error == 0) {
        // Engage Relay 2
        PORTD |= (1 << BIT_PUMP2);    
        
        if (Wait_And_Check_IR(time2_ms) == 0) { 
            glass_error = 1; 
        }
        
        // Disengage Relay 2
        PORTD &= ~(1 << BIT_PUMP2);
    }

    // --- PUMP 3 BLOCK ---
    if (time3_ms > 0 && glass_error == 0) {
        // Engage Relay 3
        PORTD |= (1 << BIT_PUMP3);    
        
        if (Wait_And_Check_IR(time3_ms) == 0) { 
            glass_error = 1; 
        }
        
        // Disengage Relay 3
        PORTD &= ~(1 << BIT_PUMP3);
    }

    // --- FINALIZATION & CLEANUP ---
    LCD_Init();

    if (glass_error == 1) {
        LCD_SetCursor(0, 0); 
        LCD_String("Oprire urgenta! ");
        LCD_SetCursor(1, 0); 
        LCD_String("Pahar indepartat");
        
        // Double-check that all relays are forced OFF
        PORTD &= ~((1 << BIT_PUMP1) | (1 << BIT_PUMP2) | (1 << BIT_PUMP3));
        
        // Keep the error message on screen for 3 seconds
        uint32_t error_start = system_ticks;
        while(system_ticks - error_start < ERROR_DISPLAY_TIME) {
            // Wait
        }
    } 
    else {
        LCD_SetCursor(0, 0); 
        LCD_String("Cocktail gata! ");
        LCD_SetCursor(1, 0); 
        LCD_String("Ridica paharul! ");
        
        // Ensure the success message is visible for at least 2 seconds
        uint32_t display_time = system_ticks;
        while (system_ticks - display_time < SUCCESS_DISPLAY_TIME) {
            // Wait
        }
        
        // Block the system until the user actually removes the glass
        while( (PIND & (1 << BIT_IR)) == 0 ) {
            // Empty loop
        }
    }
    
    // Return to IDLE state and clear the screen
    *system_state = IDLE_STATE;
    LCD_Cmd(0x01); 
}

/*
 * Main program entry point. Contains the infinite loop state machine.
 */
int main(void) {
    // Initialize all peripherals and system settings
    init();

    // Finite state machine tracker (0 = IDLE, 1 = POURING)
    uint8_t system_state = IDLE_STATE; 
    
    // Variables to hold the calculated volumes for each liquid in milliliters
    uint8_t vol1_ml = 0, vol2_ml = 0, vol3_ml = 0;

    while (1) {
        // Read the sensors and calculate volumes in milliliters
        update_volumes(&vol1_ml, &vol2_ml, &vol3_ml);

        // 2. Decide what action to take based on the current state of the system
        if (system_state == IDLE_STATE) {
            process_idle_state(vol1_ml, vol2_ml, vol3_ml, &system_state);
        } 
        else if (system_state == POURING_STATE) {
            process_pouring_state(vol1_ml, vol2_ml, vol3_ml, &system_state);
        }
    }
    
    return 0;
}
