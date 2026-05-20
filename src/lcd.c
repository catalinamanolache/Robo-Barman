#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h"
#include "i2c.h"
#include "config.h"

/*
 * Sends a 4-bit nibble of data to the I2C LCD.
 * Handles the Enable pin pulsing required by the HD44780 controller.
 */
void LCD_Write_Nibble(uint8_t nibble, uint8_t is_data) {
    // Construct the byte: nibble + RS bit + Backlight bit (0x08)
    uint8_t data = nibble | (is_data ? 1 : 0) | 0x08; 
    
    I2C_Start();
    
    // Send the I2C address shifted left by 1 (Write mode)
    I2C_Write(LCD_ADDR << 1);
    
    // Pulse the Enable (EN) pin HIGH
    I2C_Write(data | 0x04);
    
    // Pulse the Enable (EN) pin LOW to latch the data
    I2C_Write(data & ~0x04);
    
    I2C_Stop();
    
    // Small delay to allow the LCD controller to process
    _delay_us(50);
}

/*
 * Sends an instruction/command byte to the LCD.
 * Splits the 8-bit command into two 4-bit nibbles.
 */
void LCD_Cmd(uint8_t cmd) {
    // Send the upper nibble (RS = 0 for command)
    LCD_Write_Nibble(cmd & 0xF0, 0);       
    
    // Send the lower nibble (RS = 0 for command)
    LCD_Write_Nibble((cmd << 4) & 0xF0, 0); 
    
    // Standard commands need around 2ms to execute safely
    _delay_ms(2);
}

/*
 * Sends a single character byte to be printed on the LCD.
 * Splits the 8-bit character into two 4-bit nibbles.
 */
void LCD_Char(char data) {
    // Send the upper nibble (RS = 1 for data)
    LCD_Write_Nibble(data & 0xF0, 1);       
    
    // Send the lower nibble (RS = 1 for data)
    LCD_Write_Nibble((data << 4) & 0xF0, 1);
    
    // Character printing takes roughly 50us
    _delay_us(50);
}

/*
 * Initializes the LCD display in 4-bit mode.
 * Follows the strict initialization sequence from the datasheet.
 */
void LCD_Init(void) {
    // Power-on delay to let voltage stabilize
    _delay_ms(50); 
    
    // Wake up sequence and switch to 4-bit mode
    LCD_Cmd(0x33); 
    LCD_Cmd(0x32); 
    
    // Function set: 4-bit, 2 lines, 5x8 font
    LCD_Cmd(0x28); 
    
    // Display ON, Cursor OFF, Blink OFF
    LCD_Cmd(0x0C); 
    
    // Clear display completely
    LCD_Cmd(0x01); 
    
    // Clearing the display takes longer
    _delay_ms(2);
}

/*
 * Prints a null-terminated string to the LCD.
 */
void LCD_String(const char* str) {
    // Loop through the array until the null terminator is reached
    while (*str) { 
        LCD_Char(*str++); 
    }
}

/*
 * Sets the cursor position on the 16x2 LCD.
 * Row can be 0 or 1. Column can be 0 to 15.
 */
void LCD_SetCursor(uint8_t row, uint8_t col) {
    // Row 0 starts at DDRAM address 0x80. Row 1 starts at 0xC0.
    uint8_t base_address = (row == 0) ? 0x80 : 0xC0;
    
    // Send the calculated address as a command
    LCD_Cmd(base_address + col);
}
