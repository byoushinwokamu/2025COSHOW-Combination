#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>

extern volatile uint8_t tim_min;
extern volatile uint8_t tim_sec;
extern volatile uint8_t tim_msec;
extern volatile uint8_t tim_update;

void Timer1_Init(void);
void Timer1_Stop(void);
void Timer1_Resume(void);
void Timer1_Reset(void);

#endif