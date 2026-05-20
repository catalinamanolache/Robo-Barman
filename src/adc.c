#include <avr/io.h>
#include "adc.h"

/*
 * Initializes the Analog-to-Digital Converter (ADC).
 * Configures the reference voltage and the clock prescaler.
 */
void ADC_Init(void) {
    // Set reference voltage to AVCC (5V)
    ADMUX = (1 << REFS0);
    
    // Enable ADC and set the prescaler to 128 (16MHz / 128 = 125kHz ADC clock)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

/*
 * Reads the analog value from the specified ADC channel (0-7).
 * Waits for the conversion to finish and returns the 10-bit result.
 */
uint16_t ADC_Read(uint8_t channel) {
    // Clear the previous channel selection and set the new one
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    
    // Start the analog-to-digital conversion
    ADCSRA |= (1 << ADSC);
    
    // Wait in a loop until the ADSC bit is cleared by hardware (conversion done)
    while (ADCSRA & (1 << ADSC)) {
        // Wait
    }
    
    // Return the 10-bit hardware result
    return ADC;
}
