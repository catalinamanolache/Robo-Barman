#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "config.h"

/*
 * Global variable for tracking system time in milliseconds.
 * It is automatically updated by the Timer1 interrupt.
 */
volatile uint32_t system_ticks = 0;

/*
 * Configures Timer1 to generate an interrupt exactly every 1 millisecond.
 * Used for non-blocking delays and overall system timing.
 */
void Timer1_Init_Systick(void) {
    // Clear the control registers and counter
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // Set CTC (Clear Timer on Compare Match) mode
    TCCR1B |= (1 << WGM12);

    // Set Prescaler to 64
    TCCR1B |= (1 << CS11) | (1 << CS10);

    // Set the compare threshold for 1ms (16MHz / 64 / 1000 = 250 ticks)
    // We use 249 because the counter starts from 0
    OCR1A = 249;

    // Enable the Compare Match A interrupt
    TIMSK1 |= (1 << OCIE1A);
}

/*
 * Interrupt Service Routine (ISR) for Timer1 Compare Match A.
 * Executes automatically in the background every 1ms.
 */
ISR(TIMER1_COMPA_vect) {
    // Increment the global system ticks counter
    system_ticks++;
}

/*
 * Advanced non-blocking delay function that constantly monitors the IR sensor.
 * Uses a leaky integrator algorithm to filter out electromagnetic noise.
 * Returns 1 if the delay finished successfully, or 0 if the glass was removed.
 */
uint8_t Wait_And_Check_IR(uint32_t wait_time_ms) {
    // Record the starting time
    uint32_t start_time = system_ticks;
    
    // Error counter for noise filtering
    uint32_t glass_errors = 0; 
    
    // Time tracking for 1ms checks
    uint32_t last_check = system_ticks;

    // Loop until the required wait time has elapsed
    while ((system_ticks - start_time) < wait_time_ms) {
        
        // Execute the check exactly once per millisecond
        if (system_ticks != last_check) {
            last_check = system_ticks;
            
            // Read IR pin. If HIGH (1), the sensor claims the glass is missing.
            if (PIND & (1 << PIND2)) {
                glass_errors++; // Increase the panic level
                
                // If the error persists for 150 net milliseconds, it's real!
                if (glass_errors > 150) { 
                    return 0; // Trigger emergency stop
                }
            } else {
                // If LOW (0), the glass is detected. 
                // Gradually decrease panic level to filter out noise spikes.
                if (glass_errors > 0) {
                    glass_errors--;
                }
            }
        }
    }
    
    // The delay finished without critical errors
    return 1; 
}
