#ifndef LCD_H
#define LCD_H

#include <stdint.h>

void LCD_Init(void);
void LCD_Cmd(uint8_t cmd);
void LCD_Char(char data);
void LCD_String(const char* str);
void LCD_SetCursor(uint8_t row, uint8_t col);

#endif /* LCD_H */