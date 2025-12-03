#ifndef TM1650_ANIMATION_H_
#define TM1650_ANIMATION_H_

#include <stdint.h>

// --- 7-Segment 애니메이션 함수 ---

// 로딩 애니메이션 (회전하는 선)
void TM1650_AnimationLoading(uint8_t cycles, uint16_t delay_ms);

// 깜빡임 애니메이션
void TM1650_AnimationBlink(uint16_t num, uint8_t times, uint16_t delay_ms);

// 아래에서 위로 주르륵 올라가는 애니메이션 (4자리 동시)
void TM1650_AnimationScrollUp(uint8_t cycles, uint16_t delay_ms);

// 테두리를 한 바퀴 돌린 후 PASS 표시
void TM1650_AnimationSuccess_Circle(void);

// CLOSED 텍스트가 왼쪽에서 오른쪽으로 스크롤
void TM1650_AnimationScrollText_CLOSED(void);

#endif
