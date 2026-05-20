#include <avr/io.h>
#include "i2c.h"
#include "config.h"

/*
 * Initializes the I2C (Two-Wire Interface) communication.
 * Sets the bit rate register to achieve a 100 kHz clock frequency.
 */
void I2C_Init(void) {
    // Set the TWI Bit Rate Register to 72 for 100 kHz SCL frequency
    TWBR = 72; 
}

/*
 * Sends a START condition over the I2C bus.
 * Waits until the hardware confirms the transmission.
 */
void I2C_Start(void) {
    // Enable TWI, send START condition, and clear the interrupt flag
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); 
    
    // Wait in a loop until the TWINT flag is set by hardware
    while (!(TWCR & (1 << TWINT))) {
        // Wait
    }
}

/*
 * Writes one byte of data to the I2C bus.
 * Waits until the transmission is successfully completed.
 */
void I2C_Write(uint8_t data) {
    // Load the data into the TWI Data Register
    TWDR = data; 
    
    // Enable TWI and clear the interrupt flag to start transmission
    TWCR = (1 << TWINT) | (1 << TWEN); 
    
    // Wait until the TWINT flag indicates the data was sent
    while (!(TWCR & (1 << TWINT))) {
        // Wait
    }
}

/*
 * Sends a STOP condition over the I2C bus.
 * Frees the bus for other potential devices.
 */
void I2C_Stop(void) {
    // Enable TWI, send STOP condition, and clear the interrupt flag
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); 
}