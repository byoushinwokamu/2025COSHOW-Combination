#define F_CPU 8000000L

#include "tm1650_animation.h"
#include "tm1650.h"
#include <avr/io.h>
#include <util/delay.h>

// --- TM1650 명령 정의 ---
#define TM1650_DIGIT_1 0x68
#define TM1650_DIGIT_2 0x6A
#define TM1650_DIGIT_3 0x6C
#define TM1650_DIGIT_4 0x6E

// --- 7-Segment 애니메이션 함수 ---

// 로딩 애니메이션 (회전하는 선)
void TM1650_AnimationLoading(uint8_t cycles, uint16_t delay_ms)
{
  const uint8_t loading_pattern[] = {
      0x01, // 위 가로선
      0x02, // 오른쪽 위 세로선
      0x04, // 오른쪽 아래 세로선
      0x08, // 아래 가로선
      0x10, // 왼쪽 아래 세로선
      0x20  // 왼쪽 위 세로선
  };

  for (uint8_t cycle = 0; cycle < cycles; cycle++)
  {
    for (uint8_t i = 0; i < 6; i++)
    {
      TM1650_SendCommand(0x68, loading_pattern[i]);
      TM1650_SendCommand(0x6A, loading_pattern[i]);
      TM1650_SendCommand(0x6C, loading_pattern[i]);
      TM1650_SendCommand(0x6E, loading_pattern[i]);
      for (uint16_t j = 0; j < delay_ms; j++)
      {
        _delay_ms(1);
      }
    }
  }
}

// 깜빡임 애니메이션
void TM1650_AnimationBlink(uint16_t num, uint8_t times, uint16_t delay_ms)
{
  for (uint8_t i = 0; i < times; i++)
  {
    TM1650_DisplayNumber(num, 0); // leading zero 유지
    for (uint16_t j = 0; j < delay_ms; j++)
    {
      _delay_ms(1);
    }
    TM1650_SendCommand(0x68, 0x00);
    TM1650_SendCommand(0x6A, 0x00);
    TM1650_SendCommand(0x6C, 0x00);
    TM1650_SendCommand(0x6E, 0x00);
    for (uint16_t j = 0; j < delay_ms; j++)
    {
      _delay_ms(1);
    }
  }
}

// 아래에서 위로 주르륵 올라가는 애니메이션 (4자리 동시)
void TM1650_AnimationScrollUp(uint8_t cycles, uint16_t delay_ms)
{
  const uint8_t scroll_pattern[] = {
      0x08,        // 1. 아래 가로선 (___)
      0x04 | 0x10, // 2. 왼쪽 오른쪽 아래 세로선 (|_|)
      0x40,        // 3. 중간 가로선 (―)
      0x02 | 0x20, // 4. 왼쪽 오른쪽 위 세로선 (|¯|)
      0x01         // 5. 위 가로선 (¯¯¯)
  };

  for (uint8_t cycle = 0; cycle < cycles; cycle++)
  {
    for (uint8_t i = 0; i < 5; i++)
    {
      // 4자리 모두 같은 패턴 표시
      TM1650_SendCommand(0x68, scroll_pattern[i]);
      TM1650_SendCommand(0x6A, scroll_pattern[i]);
      TM1650_SendCommand(0x6C, scroll_pattern[i]);
      TM1650_SendCommand(0x6E, scroll_pattern[i]);
      for (uint16_t j = 0; j < delay_ms; j++)
      {
        _delay_ms(1);
      }
    }
  }
}

// 테두리를 한 바퀴 돌린 후 PASS 표시
void TM1650_AnimationSuccess_Circle(void)
{
  TM1650_Clear();
  uint16_t speed = 80; // 회전 속도

  // 위쪽 (왼->오)
  TM1650_SendCommand(TM1650_DIGIT_1, 0x01);
  _delay_ms(speed);
  TM1650_SendCommand(TM1650_DIGIT_2, 0x01);
  _delay_ms(speed);
  TM1650_SendCommand(TM1650_DIGIT_3, 0x01);
  _delay_ms(speed);
  TM1650_SendCommand(TM1650_DIGIT_4, 0x01);
  _delay_ms(speed);

  // 오른쪽 (위->아래)
  TM1650_SendCommand(TM1650_DIGIT_4, 0x01 | 0x02);
  _delay_ms(speed); // A+B
  TM1650_SendCommand(TM1650_DIGIT_4, 0x01 | 0x02 | 0x04);
  _delay_ms(speed); // A+B+C

  // 아래쪽 (오->왼)
  TM1650_SendCommand(TM1650_DIGIT_4, 0x01 | 0x02 | 0x04 | 0x08);
  _delay_ms(speed); // +D
  TM1650_SendCommand(TM1650_DIGIT_3, 0x01 | 0x08);
  _delay_ms(speed);
  TM1650_SendCommand(TM1650_DIGIT_2, 0x01 | 0x08);
  _delay_ms(speed);
  TM1650_SendCommand(TM1650_DIGIT_1, 0x01 | 0x08);
  _delay_ms(speed);

  // 왼쪽 (아래->위) 및 전체 채우기
  TM1650_SendCommand(TM1650_DIGIT_1, 0x01 | 0x08 | 0x10);
  _delay_ms(speed); // +E
  TM1650_SendCommand(TM1650_DIGIT_1, 0x01 | 0x08 | 0x10 | 0x20);
  _delay_ms(speed); // +F

  // 최종 PASS 표시
  TM1650_Clear();
  TM1650_SendCommand(TM1650_DIGIT_1, 0x73); // P
  TM1650_SendCommand(TM1650_DIGIT_2, 0x77); // A
  TM1650_SendCommand(TM1650_DIGIT_3, 0x6D); // S
  TM1650_SendCommand(TM1650_DIGIT_4, 0x6D); // S
  _delay_ms(500);
}

// CLOSED 텍스트가 왼쪽에서 오른쪽으로 스크롤
void TM1650_AnimationScrollText_CLOSED(void)
{
  // C, L, O, S, E, D 세그먼트 패턴
  const uint8_t closed_text[] = {
      0x39, // C
      0x38, // L
      0x3F, // O
      0x6D, // S
      0x79, // E
      0x5E  // d
  };

  uint16_t delay = 200; // 스크롤 속도

  // 6글자를 오른쪽으로 스크롤 (총 10프레임)
  for (uint8_t frame = 0; frame < 10; frame++)
  {
    TM1650_Clear();

    // 각 프레임에서 표시할 문자 결정
    for (uint8_t digit = 0; digit < 4; digit++)
    {
      int8_t text_index = frame - 3 + digit;

      if (text_index >= 0 && text_index < 6)
      {
        uint8_t addr = 0x68 + (digit * 2);
        TM1650_SendCommand(addr, closed_text[text_index]);
      }
    }

    _delay_ms(delay);
  }

  TM1650_Clear();
}
