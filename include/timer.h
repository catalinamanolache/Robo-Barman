#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>

extern volatile uint32_t system_ticks;

void Timer1_Init_Systick(void);
uint8_t Wait_And_Check_IR(uint32_t wait_time_ms);

#endif /* TIMER_H */