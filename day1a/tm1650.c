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
    0x6F  // 9
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
  return key;
}
