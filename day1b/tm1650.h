#ifndef TM1650_H_
#define TM1650_H_

#include <stdint.h>

void TM1650_SendCommand(uint8_t cmd, uint8_t data);
void TM1650_Init();
void TM1650_DisplayNumber(uint16_t num, uint8_t remove_leading_zero);
void TM1650_DisplayNumber_safe(uint16_t num, uint8_t digit_count);
void TM1650_DisplayHex(uint16_t num, uint8_t remove_leading_zero);
void TM1650_Clear(void);
void TM1650_SetDecimalPoint(uint8_t digit, uint8_t dp_on);
void TM1650_DisplayNumberWithDP(uint16_t num, uint8_t dp_position);
void TM1650_DisplayTimeWithBlink(uint8_t min, uint8_t sec, uint8_t msec, uint8_t min_dp, uint8_t blink_digit);

// 키패드 입력 함수
uint8_t TM1650_ReadKeycode(void);  // TM1650 스캔코드 읽기 (0x44~0x77)
uint8_t TM1650_GetKeyNumber(void); // 키 번호로 변환 (1~16, 0=입력없음)
char TM1650_GetKeyChar(void);      // 키 문자로 변환 ('0'~'9','A'~'D','*','#')

#endif