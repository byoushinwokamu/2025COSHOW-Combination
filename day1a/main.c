#define F_CPU 8000000L

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "buzzer.h"
#include "i2c.h"
#include "timer.h"
#include "tm1650.h"

int main()
{
  volatile uint8_t key, pastkey = 0;

  // init phase
  DDRB |= (1 << 3) | (1 << 4);
  PORTB &= (1 << 4);
  PORTB |= (1 << 3); // initially stopwatch mode
  I2C_Init();
  Timer1_Init();
  TM1650_Init();
  buzzer_init();

  while (1)
  {
    if (tim_update) // 시간 변경사항 있을 시 디스플레이 업데이트
    {
      TM1650_DisplayTimeWithBlink(tim_min, tim_sec, tim_msec, 1, 3);
      tim_update = 0;
    }

    key = TM1650_ReadKeycode(); // Key scan

    if (key != pastkey)
    {
      switch (key)
      {
      case 0x44: // SW1: Start & Resume / Start & Pause & Resume
        if (tim_mode == MODE_STOPWATCH)
          Timer1_Resume();
        else
        {
          if (tim_enable) Timer1_Stop();
          else if (!(tim_min & tim_sec & tim_msec)) Timer1_Resume();
        }
        break;

      case 0x45: // SW2: Pause / Time Set
        if (tim_mode == MODE_STOPWATCH)
          Timer1_Stop();
        else
        {
          if ((tim_sec += 5) >= 60)
          {
            tim_sec -= 60;
            if (++tim_min >= 9) tim_min = 9, tim_sec = 55;
          }
          tim_update = 1;
        }
        break;

      case 0x46: // SW3: Reset
        Timer1_Reset();
        tim_update = 1;
        break;

      case 0x47: // SW4: Mode change
        tim_mode = (tim_mode == MODE_STOPWATCH) ? MODE_TIMEBOMB : MODE_STOPWATCH;
        PORTB ^= (1 << 3) | (1 << 4);
        Timer1_Stop();
        Timer1_Reset();
        tim_update = 1;
        break;

      default: // no switch pushed
        break;
      }
    }

    if (tim_expired) // 시한폭탄 폭발!
    {
      play_mario_death();
      tim_expired = 0;
    }

    pastkey = key;
  }
}