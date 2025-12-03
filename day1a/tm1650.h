#ifndef _TM1650_H_
#define _TM1650_H_

#include <stdint.h>

void TM1650_SendCommand(uint8_t cmd, uint8_t data);
void TM1650_Init();

// 시간 표시 + 특정 자리 소수점 깜빡임
// min: 0~9분, sec: 0~59초, msec: 0~9 (0.1초 단위)
// min_dp: 1번 자리(분) 소수점 (0=끄기, 1=켜기)
// blink_digit: 깜빡일 자리 (1~4), 0이면 깜빡임 없음
// msec 값 기준으로 1초 주기 자동 깜빡임 (0~4: 켜짐, 5~9: 꺼짐)
void TM1650_DisplayTimeWithBlink(uint8_t min, uint8_t sec, uint8_t msec, uint8_t min_dp, uint8_t blink_digit);

uint8_t TM1650_ReadKeycode(void); // TM1650 스캔코드 읽기 (0x44~0x77)

#endif