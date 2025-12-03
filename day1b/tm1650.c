#define F_CPU 8000000L

#include "tm1650.h"
#include "i2c.h"
#include <avr/io.h>
#include <util/delay.h>

// --- TM1650 명령 정의 ---
#define TM1650_DIGIT_1 0x68
#define TM1650_DIGIT_2 0x6A
#define TM1650_DIGIT_3 0x6C
#define TM1650_DIGIT_4 0x6E

const uint8_t SEG_FONT[] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9
    0x77, // A
    0x7C, // b
    0x39, // C
    0x5E, // d
    0x79, // E
    0x71  // F
};

// --- TM1650 제어 함수 ---

void TM1650_SendCommand(uint8_t cmd, uint8_t data)
{
  I2C_Start();
  I2C_WriteByte(cmd);
  I2C_WriteByte(data);
  I2C_Stop();
}

void TM1650_Init()
{
  I2C_Init();
  // 디스플레이 켜기, 밝기 설정 (0x01 = 1단계, 0x71 = 8단계 등 데이터시트 참조)
  // 여기서는 0x48 커맨드에 0x11 (Display ON, 밝기 3단계 정도) 전송
  TM1650_SendCommand(0x48, 0x11);
}

void TM1650_DisplayNumber(uint16_t num, uint8_t remove_leading_zero)
{
  uint8_t digits[4];

  // 숫자를 각 자리수로 분리
  digits[0] = (num / 1000) % 10;
  digits[1] = (num / 100) % 10;
  digits[2] = (num / 10) % 10;
  digits[3] = num % 10;

  if (remove_leading_zero)
  {
    // 상위 자리수가 0이면 끄기 (leading zero 제거)
    if (num < 1000)
    {
      TM1650_SendCommand(0x68, 0x00);
    }
    else
    {
      TM1650_SendCommand(0x68, SEG_FONT[digits[0]]);
    }

    if (num < 100)
    {
      TM1650_SendCommand(0x6A, 0x00);
    }
    else
    {
      TM1650_SendCommand(0x6A, SEG_FONT[digits[1]]);
    }

    if (num < 10)
    {
      TM1650_SendCommand(0x6C, 0x00);
    }
    else
    {
      TM1650_SendCommand(0x6C, SEG_FONT[digits[2]]);
    }

    TM1650_SendCommand(0x6E, SEG_FONT[digits[3]]);
  }
  else
  {
    // 모든 자리수 표시 (0000 형식)
    TM1650_SendCommand(0x68, SEG_FONT[digits[0]]);
    TM1650_SendCommand(0x6A, SEG_FONT[digits[1]]);
    TM1650_SendCommand(0x6C, SEG_FONT[digits[2]]);
    TM1650_SendCommand(0x6E, SEG_FONT[digits[3]]);
  }
}

void TM1650_DisplayNumber_safe(uint16_t num, uint8_t digit_count)
{
  uint8_t digits[4];

  // 숫자를 각 자리수로 분리
  digits[0] = (num / 1000) % 10;
  digits[1] = (num / 100) % 10;
  digits[2] = (num / 10) % 10;
  digits[3] = num % 10;

  // digit_count에 따라 표시할 자리수 결정
  // digit_count = 0: 모두 끔, 1: 마지막 1자리, 2: 마지막 2자리, ..., 4 이상: 모두 표시
  if (digit_count == 0)
  {
    // 모두 끔
    TM1650_SendCommand(0x68, 0x00);
    TM1650_SendCommand(0x6A, 0x00);
    TM1650_SendCommand(0x6C, 0x00);
    TM1650_SendCommand(0x6E, 0x00);
  }
  else if (digit_count == 1)
  {
    // 마지막 1자리만 표시
    TM1650_SendCommand(0x68, 0x00);
    TM1650_SendCommand(0x6A, 0x00);
    TM1650_SendCommand(0x6C, 0x00);
    TM1650_SendCommand(0x6E, SEG_FONT[digits[3]]);
  }
  else if (digit_count == 2)
  {
    // 마지막 2자리 표시
    TM1650_SendCommand(0x68, 0x00);
    TM1650_SendCommand(0x6A, 0x00);
    TM1650_SendCommand(0x6C, SEG_FONT[digits[2]]);
    TM1650_SendCommand(0x6E, SEG_FONT[digits[3]]);
  }
  else if (digit_count == 3)
  {
    // 마지막 3자리 표시
    TM1650_SendCommand(0x68, 0x00);
    TM1650_SendCommand(0x6A, SEG_FONT[digits[1]]);
    TM1650_SendCommand(0x6C, SEG_FONT[digits[2]]);
    TM1650_SendCommand(0x6E, SEG_FONT[digits[3]]);
  }
  else
  {
    // 4자리 모두 표시 (digit_count >= 4)
    TM1650_SendCommand(0x68, SEG_FONT[digits[0]]);
    TM1650_SendCommand(0x6A, SEG_FONT[digits[1]]);
    TM1650_SendCommand(0x6C, SEG_FONT[digits[2]]);
    TM1650_SendCommand(0x6E, SEG_FONT[digits[3]]);
  }
}

// 16진수 표시 함수 (0x0000 ~ 0xFFFF)
void TM1650_DisplayHex(uint16_t num, uint8_t remove_leading_zero)
{
  uint8_t digits[4];

  // 16진수 각 자리수로 분리
  digits[0] = (num >> 12) & 0x0F; // 최상위 4비트
  digits[1] = (num >> 8) & 0x0F;  // 상위 4비트
  digits[2] = (num >> 4) & 0x0F;  // 하위 4비트
  digits[3] = num & 0x0F;         // 최하위 4비트

  if (remove_leading_zero)
  {
    // 상위 자리수가 0이면 끄기
    if (num < 0x1000)
    {
      TM1650_SendCommand(0x68, 0x00);
    }
    else
    {
      TM1650_SendCommand(0x68, SEG_FONT[digits[0]]);
    }

    if (num < 0x0100)
    {
      TM1650_SendCommand(0x6A, 0x00);
    }
    else
    {
      TM1650_SendCommand(0x6A, SEG_FONT[digits[1]]);
    }

    if (num < 0x0010)
    {
      TM1650_SendCommand(0x6C, 0x00);
    }
    else
    {
      TM1650_SendCommand(0x6C, SEG_FONT[digits[2]]);
    }

    // 최하위 자리는 항상 표시
    TM1650_SendCommand(0x6E, SEG_FONT[digits[3]]);
  }
  else
  {
    // 모든 자리수 표시 (0000 형식)
    TM1650_SendCommand(0x68, SEG_FONT[digits[0]]);
    TM1650_SendCommand(0x6A, SEG_FONT[digits[1]]);
    TM1650_SendCommand(0x6C, SEG_FONT[digits[2]]);
    TM1650_SendCommand(0x6E, SEG_FONT[digits[3]]);
  }
}

// 화면 전체 지우기 (모든 세그먼트 끄기)
void TM1650_Clear(void)
{
  TM1650_SendCommand(0x68, 0x00);
  TM1650_SendCommand(0x6A, 0x00);
  TM1650_SendCommand(0x6C, 0x00);
  TM1650_SendCommand(0x6E, 0x00);
}

// 소수점 제어 함수
// digit: 자릿수 (1~4), dp_on: 1이면 켜기, 0이면 끄기
void TM1650_SetDecimalPoint(uint8_t digit, uint8_t dp_on)
{
  uint8_t addr;

  // 자릿수에 따른 주소 설정
  switch (digit)
  {
  case 1:
    addr = TM1650_DIGIT_1;
    break;
  case 2:
    addr = TM1650_DIGIT_2;
    break;
  case 3:
    addr = TM1650_DIGIT_3;
    break;
  case 4:
    addr = TM1650_DIGIT_4;
    break;
  default:
    return; // 잘못된 자릿수
  }

  // 현재 표시 중인 숫자에 DP 비트(0x80) 추가/제거
  // 주의: 이 함수는 현재 표시된 내용을 유지하지 않으므로
  // 숫자와 함께 사용하려면 TM1650_DisplayNumberWithDP 사용
  if (dp_on)
  {
    TM1650_SendCommand(addr, 0x80); // DP만 켜기
  }
  else
  {
    TM1650_SendCommand(addr, 0x00); // DP 끄기
  }
}

// 숫자와 소수점을 함께 표시하는 함수
// dp_position: 소수점 위치 (1~4), 0이면 소수점 없음
void TM1650_DisplayNumberWithDP(uint16_t num, uint8_t dp_position)
{
  uint8_t digits[4];

  // 숫자를 각 자리수로 분리
  digits[0] = (num / 1000) % 10;
  digits[1] = (num / 100) % 10;
  digits[2] = (num / 10) % 10;
  digits[3] = num % 10;

  // 각 자리에 데이터 전송 (소수점 포함)
  uint8_t data;

  data = SEG_FONT[digits[0]];
  if (dp_position == 1) data |= 0x80; // 소수점 추가
  TM1650_SendCommand(TM1650_DIGIT_1, data);

  data = SEG_FONT[digits[1]];
  if (dp_position == 2) data |= 0x80;
  TM1650_SendCommand(TM1650_DIGIT_2, data);

  data = SEG_FONT[digits[2]];
  if (dp_position == 3) data |= 0x80;
  TM1650_SendCommand(TM1650_DIGIT_3, data);

  data = SEG_FONT[digits[3]];
  if (dp_position == 4) data |= 0x80;
  TM1650_SendCommand(TM1650_DIGIT_4, data);
}

// 시간 표시 + 특정 자리 소수점 깜빡임
// min: 0~9분, sec: 0~59초, msec: 0~9 (0.1초 단위)
// min_dp: 1번 자리(분) 소수점 (0=끄기, 1=켜기)
// blink_digit: 깜빡일 자리 (1~4), 0이면 깜빡임 없음
// msec 값 기준으로 1초 주기 자동 깜빡임 (0~4: 켜짐, 5~9: 꺼짐)
void TM1650_DisplayTimeWithBlink(uint8_t min, uint8_t sec, uint8_t msec,
                                 uint8_t min_dp, uint8_t blink_digit)
{
  // sec를 10초 단위와 1초 단위로 분리
  uint8_t tens_sec = sec / 10; // 10초 자리 (0~5)
  uint8_t ones_sec = sec % 10; // 1초 자리 (0~9)

  // msec 값으로 깜빡임 상태 결정 (0~4: 켜짐, 5~9: 꺼짐)
  uint8_t blink_on = (msec < 5) ? 1 : 0;

  uint8_t digits[4] = {min, tens_sec, ones_sec, msec};
  uint8_t data;

  // 1번 자리 (분)
  data = SEG_FONT[digits[0]];
  if (min_dp) data |= 0x80; // min_dp 설정에 따라 고정
  if (blink_digit == 1 && blink_on) data |= 0x80;
  TM1650_SendCommand(TM1650_DIGIT_1, data);

  // 2번 자리 (10초)
  data = SEG_FONT[digits[1]];
  if (blink_digit == 2 && blink_on) data |= 0x80;
  TM1650_SendCommand(TM1650_DIGIT_2, data);

  // 3번 자리 (1초)
  data = SEG_FONT[digits[2]];
  if (blink_digit == 3 && blink_on) data |= 0x80;
  TM1650_SendCommand(TM1650_DIGIT_3, data);

  // 4번 자리 (0.1초)
  data = SEG_FONT[digits[3]];
  if (blink_digit == 4 && blink_on) data |= 0x80;
  TM1650_SendCommand(TM1650_DIGIT_4, data);
}

uint8_t TM1650_ReadKeycode(void)
{
  uint8_t key;
  I2C_Start();
  I2C_WriteByte(0b01001001);
  key = I2C_ReadByte(0);
  I2C_Stop();

  // 키 스캔 후 하드웨어 안정화를 위한 짧은 지연
  // _delay_us(100);

  return key;
}

// TM1650 스캔코드를 4x4 키패드 키 번호로 변환
// 반환값: 1~16 (키 번호), 0 (키 입력 없음 또는 에러)
//
// 하드웨어 연결:
// - 키패드 열 4개: SEG2, SEG3, SEG4, SEG5 (TM1650)
// - 키패드 행 4개: GRID1, GRID2, GRID3, GRID4 (TM1650)
//
// 스캔코드 구조: [0 1 G2 G1 G0 S2 S1 S0]
// - G2,G1,G0: GRID 번호 (000=GRID1, 001=GRID2, 010=GRID3, 011=GRID4)
// - S2,S1,S0: SEG 인덱스 (100=SEG2, 101=SEG3, 110=SEG4, 111=SEG5)
//
// SEG 인덱스 (데이터시트 기준):
// - SEG2 (KI2) = 100₂ = 4
// - SEG3 (KI3) = 101₂ = 5
// - SEG4 (KI4) = 110₂ = 6
// - SEG5 (KI5) = 111₂ = 7
uint8_t TM1650_GetKeyNumber(void)
{
  uint8_t scancode = TM1650_ReadKeycode();

  // 스캔코드 매핑 테이블 (4x4 키패드)
  // SEG2~SEG5 → 열1~4, GRID1~GRID4 → 행1~4
  switch (scancode)
  {
  // GRID1 (행1) - SEG2~SEG5 (열1~4): 키 1, 2, 3, A
  case 0x4C:
    return 1; // GRID1 + SEG2 → 키 1
  case 0x54:
    return 2; // GRID1 + SEG3 → 키 2
  case 0x5C:
    return 3; // GRID1 + SEG4 → 키 3
  case 0x64:
    return 4; // GRID1 + SEG5 → 키 A

  // GRID2 (행2) - SEG2~SEG5 (열1~4): 키 4, 5, 6, B
  case 0x4D:
    return 5; // GRID2 + SEG2 → 키 4
  case 0x55:
    return 6; // GRID2 + SEG3 → 키 5
  case 0x5D:
    return 7; // GRID2 + SEG4 → 키 6
  case 0x65:
    return 8; // GRID2 + SEG5 → 키 B

  // GRID3 (행3) - SEG2~SEG5 (열1~4): 키 7, 8, 9, C
  case 0x4E:
    return 9; // GRID3 + SEG2 → 키 7
  case 0x56:
    return 10; // GRID3 + SEG3 → 키 8
  case 0x5E:
    return 11; // GRID3 + SEG4 → 키 9
  case 0x66:
    return 12; // GRID3 + SEG5 → 키 C

  // GRID4 (행4) - SEG2~SEG5 (열1~4): 키 *, 0, #, D
  case 0x4F:
    return 13; // GRID4 + SEG2 → 키 *
  case 0x57:
    return 14; // GRID4 + SEG3 → 키 0
  case 0x5F:
    return 15; // GRID4 + SEG4 → 키 #
  case 0x67:
    return 16; // GRID4 + SEG5 → 키 D

  default:
    return 0; // 키 입력 없음
  }
}

// TM1650 스캔코드를 4x4 키패드 문자로 변환
// 반환값: '0'~'9', 'A'~'D', '*', '#', 0 (키 입력 없음)
char TM1650_GetKeyChar(void)
{
  uint8_t key_num = TM1650_GetKeyNumber();

  // 키 번호를 문자로 변환
  const char key_map[17] = {
      0,                  // 0: 입력 없음
      '1', '2', '3', 'A', // 1~4: 1행
      '4', '5', '6', 'B', // 5~8: 2행
      '7', '8', '9', 'C', // 9~12: 3행
      '*', '0', '#', 'D'  // 13~16: 4행
  };

  if (key_num >= 1 && key_num <= 16)
  {
    return key_map[key_num];
  }
  else
  {
    return 0; // 키 입력 없음
  }
}