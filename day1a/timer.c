#define F_CPU 8000000UL

#include "timer.h"
#include <avr/interrupt.h>
#include <avr/io.h>

volatile uint8_t tim_min;
volatile uint8_t tim_sec;
volatile uint8_t tim_msec;
volatile uint8_t tim_update;
volatile uint8_t tim_mode = MODE_STOPWATCH;
volatile uint8_t tim_enable;
volatile uint8_t tim_expired;

void Timer1_Init(void)
{
  // CTC mode
  TCCR1 = (1 << CTC1);

  // Prescaler = 4096  (CS13=1, CS12=1, CS11=0, CS10=1)
  TCCR1 |= (1 << CS13) | (1 << CS12) | (1 << CS10);

  // OCR1C = 195 → 약 100ms
  OCR1C = 195;

  // 인터럽트 활성화
  TIMSK |= (1 << OCIE1A);
  sei();
}

volatile uint8_t tick = 0;

ISR(TIMER1_COMPA_vect)
{
  if (!tim_enable) return;

  if (tim_mode == MODE_STOPWATCH) // 스탑워치: 시간 증가
  {
    if (++tim_msec == 10)
    {
      tim_msec = 0;
      if (++tim_sec == 60)
      {
        tim_sec = 0;
        if (++tim_min == 10) tim_min = 0; // 9분 59초 90 초과: 0초로 초기화
      }
    }
  }
  else // 시한폭탄: 시간 감소
  {
    if (tim_msec-- == 0)
    {
      tim_msec = 9;
      if (tim_sec-- == 0)
      {
        tim_sec = 59;
        if (tim_min-- == 0) // 만료
        {
          tim_msec = tim_sec = tim_min = 0;
          tim_expired = 1;
          tim_enable = 0;
        }
      }
    }
  }
  tim_update = 1;
}

void Timer1_Stop(void)
{
  tim_enable = 0;
}

void Timer1_Resume(void)
{
  tim_enable = 1;
}

void Timer1_Reset(void)
{
  tim_enable = 0;
  tim_min = tim_sec = tim_msec = 0;
  TCNT1 = 0;
}