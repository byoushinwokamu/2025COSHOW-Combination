#define F_CPU 8000000L

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "buzzer.h"
#include "i2c.h"
#include "timer.h"
#include "tm1650.h"
#include "tm1650_animation.h"

// 호텔 금고 상태 정의
typedef enum
{
  STATE_INIT,         // 초기 상태 (비밀번호 미설정)
  STATE_SET_PASSWORD, // 비밀번호 설정 모드
  STATE_LOCKED,       // 잠금 상태
  STATE_UNLOCKED      // 열림 상태
} SafeState;

int main()
{
  TM1650_Init();
  _delay_ms(500);

  buzzer_init();

  // 호텔 금고 변수
  SafeState state = STATE_INIT;
  uint16_t password = 0;    // 설정된 비밀번호 (최대 4자리)
  uint16_t input_value = 0; // 현재 입력 중인 값
  uint8_t input_count = 0;  // 입력된 자릿수

  // 키 입력 관련 변수
  uint8_t curr_key_num = 0;
  uint8_t prev_key_num = 0;
  char key_char = 0;

  // 시작 애니메이션
  TM1650_AnimationLoading(2, 100);
  TM1650_Clear();
  _delay_ms(500);

  // 초기 상태: 비밀번호 설정 요청 (----로 표시)
  TM1650_SendCommand(0x68, 0x40); // -
  TM1650_SendCommand(0x6A, 0x40); // -
  TM1650_SendCommand(0x6C, 0x40); // -
  TM1650_SendCommand(0x6E, 0x40); // -

  while (1)
  {
    _delay_ms(1);
    // 키 스캔
    curr_key_num = TM1650_GetKeyNumber();

    // Rising Edge 감지
    if (curr_key_num != 0 && prev_key_num == 0)
    {
      key_char = TM1650_GetKeyChar();

      // === STATE_INIT: 초기 비밀번호 설정 ===
      if (state == STATE_INIT || state == STATE_SET_PASSWORD)
      {
        if (key_char >= '0' && key_char <= '9')
        {
          if (input_count < 4)
          {
            input_value = ((input_value % 10000) * 10 + (key_char - '0')) % 10000;
            input_count++;
            TM1650_DisplayNumber_safe(input_value, input_count);
            play_coin();
          }
        }
        else if (key_char == '*')
        {
          // Backspace
          if (input_count > 0)
          {
            input_count--;
            if (input_count == 0)
            {
              input_value = 0;
              TM1650_Clear();
            }
            else
            {
              input_value = input_value / 10;
              TM1650_DisplayNumber_safe(input_value, input_count);
            }
          }
        }
        else if (key_char == '#')
        {
          // 비밀번호 확정
          if (input_count >= 1)
          {
            password = input_value;
            state = STATE_LOCKED;

            // 설정 완료 애니메이션
            TM1650_AnimationBlink(password, 3, 200);
            play_mario_1up();
            TM1650_Clear();

            // "CLOSED" 스크롤 애니메이션
            TM1650_AnimationScrollText_CLOSED();
            _delay_ms(300);

            TM1650_Clear();

            input_value = 0;
            input_count = 0;
          }
        }
        else if (key_char == 'A')
        {
          // 입력 지우기
          input_value = 0;
          input_count = 0;
          TM1650_Clear();
        }
      }

      // === STATE_LOCKED: 잠금 상태 (비밀번호 입력 대기) ===
      else if (state == STATE_LOCKED)
      {
        if (key_char >= '0' && key_char <= '9')
        {
          if (input_count < 4)
          {
            input_value = ((input_value % 10000) * 10 + (key_char - '0')) % 10000;
            input_count++;
            TM1650_DisplayNumber_safe(input_value, input_count);
            play_coin();
          }
        }
        else if (key_char == '*')
        {
          // Backspace
          if (input_count > 0)
          {
            input_count--;
            if (input_count == 0)
            {
              input_value = 0;
              TM1650_Clear();
            }
            else
            {
              input_value = input_value / 10;
              TM1650_DisplayNumber_safe(input_value, input_count);
            }
          }
        }
        else if (key_char == '#')
        {
          // 비밀번호 확인
          if (input_value == password)
          {
            // 비밀번호 일치 - 금고 열림
            state = STATE_UNLOCKED;

            // 성공 애니메이션
            TM1650_AnimationSuccess_Circle();
            play_zelda_secret();
            // TM1650_Clear();
            _delay_ms(500);

            // "OPEN" 표시
            TM1650_SendCommand(0x68, 0x3F); // O
            TM1650_SendCommand(0x6A, 0x73); // P
            TM1650_SendCommand(0x6C, 0x79); // E
            TM1650_SendCommand(0x6E, 0x37); // N
            // _delay_ms(2000);
            // TM1650_Clear();

            input_value = 0;
            input_count = 0;
          }
          else
          {
            // 비밀번호 불일치 - 실패 애니메이션
            TM1650_AnimationBlink(input_value, 5, 100);
            TM1650_Clear();

            // "FAIL" 표시
            TM1650_SendCommand(0x68, 0x71); // F
            TM1650_SendCommand(0x6A, 0x77); // A
            TM1650_SendCommand(0x6C, 0x06); // I
            TM1650_SendCommand(0x6E, 0x38); // L
            play_error_sound();
            _delay_ms(1500);
            TM1650_Clear();

            input_value = 0;
            input_count = 0;
          }
        }
        else if (key_char == 'A')
        {
          // 입력 지우기
          input_value = 0;
          input_count = 0;
          TM1650_Clear();
        }
        else if (key_char == 'D')
        {
          // 비밀번호 재설정 모드 진입 (특수 기능)
          state = STATE_SET_PASSWORD;
          TM1650_Clear();

          // "RSET" 표시
          TM1650_SendCommand(0x68, 0x50); // r
          TM1650_SendCommand(0x6A, 0x6D); // S
          TM1650_SendCommand(0x6C, 0x79); // E
          TM1650_SendCommand(0x6E, 0x78); // t (소문자)
          _delay_ms(1500);

          // "----" 표시 (입력 대기)
          TM1650_SendCommand(0x68, 0x40); // -
          TM1650_SendCommand(0x6A, 0x40); // -
          TM1650_SendCommand(0x6C, 0x40); // -
          TM1650_SendCommand(0x6E, 0x40); // -

          input_value = 0;
          input_count = 0;
        }
      }

      // === STATE_UNLOCKED: 열림 상태 ===
      else if (state == STATE_UNLOCKED)
      {
        if (key_char == '#')
        {
          // 금고 다시 잠그기
          state = STATE_LOCKED;

          // 위로 스크롤 애니메이션
          TM1650_AnimationScrollUp(3, 80);
          TM1650_Clear();

          // "CLOSED" 스크롤 애니메이션
          TM1650_AnimationScrollText_CLOSED();
          _delay_ms(300);
          TM1650_Clear();

          input_value = 0;
          input_count = 0;
        }
        else if (key_char == 'B')
        {
          // 전체 초기화 (비밀번호 삭제)
          state = STATE_INIT;
          password = 0;
          input_value = 0;
          input_count = 0;

          // 초기화 애니메이션
          TM1650_AnimationScrollUp(3, 80);
          TM1650_Clear();

          // "RSET" 표시
          TM1650_SendCommand(0x68, 0x50); // r
          TM1650_SendCommand(0x6A, 0x6D); // S
          TM1650_SendCommand(0x6C, 0x79); // E
          TM1650_SendCommand(0x6E, 0x78); // t (소문자)
          _delay_ms(1500);

          // "----" 표시
          TM1650_SendCommand(0x68, 0x40);
          TM1650_SendCommand(0x6A, 0x40);
          TM1650_SendCommand(0x6C, 0x40);
          TM1650_SendCommand(0x6E, 0x40);
        }
        else if (key_char == 'C')
        {
          // 현재 비밀번호 표시 (디버그/확인용)
          TM1650_DisplayNumber_safe(password, 4);
          _delay_ms(2000);
          TM1650_Clear();

          // "OPEN" 표시로 복귀
          TM1650_SendCommand(0x68, 0x3F); // O
          TM1650_SendCommand(0x6A, 0x73); // P
          TM1650_SendCommand(0x6C, 0x79); // E
          TM1650_SendCommand(0x6E, 0x37); // N
        }
      }

      // 화면 업데이트 (숫자 입력시 이미 표시되므로 STATE_INIT/STATE_SET_PASSWORD는 제외)
      if (state == STATE_LOCKED)
      {
        // 잠금 상태에서는 입력값이 있을 때만 표시
        if (input_count > 0)
        {
          TM1650_DisplayNumber_safe(input_value, input_count);
        }
      }
      else if (state == STATE_INIT || state == STATE_SET_PASSWORD)
      {
        // 초기 상태에서는 입력값 표시 (숫자 입력시 이미 표시됨)
        if (input_count > 0)
        {
          TM1650_DisplayNumber_safe(input_value, input_count);
        }
      }
    }

    prev_key_num = curr_key_num;
  }
}