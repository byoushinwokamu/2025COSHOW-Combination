#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>

// 시간 변수
extern volatile uint8_t tim_min;
extern volatile uint8_t tim_sec;
extern volatile uint8_t tim_msec;

extern volatile uint8_t tim_update;  // 시간 변경됨 플래그
extern volatile uint8_t tim_mode;    // 스탑워치 / 시한폭탄
extern volatile uint8_t tim_enable;  // 진행 / 정지
extern volatile uint8_t tim_expired; // 시한폭탄 만료

#define MODE_STOPWATCH 1
#define MODE_TIMEBOMB 2

void Timer1_Init(void);
void Timer1_Stop(void);
void Timer1_Resume(void);
void Timer1_Reset(void);

#endif