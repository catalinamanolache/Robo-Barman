#ifndef I2C_H
#define I2C_H

#include <stdint.h>

void I2C_Init(void);
void I2C_Start(void);
void I2C_Write(uint8_t data);
void I2C_Stop(void);

#endif /* I2C_H */