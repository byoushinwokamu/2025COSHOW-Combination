/*
 * buzzer.h
 * 부저 제어 및 효과음 관련 선언부
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <avr/io.h>

// ============================================================================
// 1. 하드웨어 핀 정의
// ============================================================================
// 부저 핀: D12 (PB4)
#define BUZZER_DDR DDRB
#define BUZZER_PORT PORTB
#define BUZZER_PIN PB4

// ============================================================================
// 2. 주파수 정의 (Prescaler 64 기준 OCR 값)
// ============================================================================
#define NOTE_C4 238
#define NOTE_G4 158
#define NOTE_A4 141
#define NOTE_B4 126

#define NOTE_C5 118
#define NOTE_D5 105
#define NOTE_E5 94
#define NOTE_F5 88
#define NOTE_G5 79
#define NOTE_GS5 75 // G#5
#define NOTE_A5 70
#define NOTE_B5 62

#define NOTE_C6 59
#define NOTE_D6 52
#define NOTE_E6 46
#define NOTE_FS6 41 // F#6
#define NOTE_G6 39
#define NOTE_C7 29

#define MUTE 0 // 무음

// ============================================================================
// 3. 함수 프로토타입 (외부에서 사용할 함수들)
// ============================================================================

// 초기화 및 제어
void buzzer_init(void);
void buzzer_tone(uint8_t ocr_value);
void buzzer_stop(void);

// Non-blocking 부저 제어
void buzzer_update(void);   // 매 프레임마다 호출 필요
void play_coin_start(void); // 코인 소리 시작 (non-blocking)
void play_jump_start(void); // 점프 소리 시작 (non-blocking)

// 효과음 재생 함수 (blocking - delay 사용)
void play_coin(void);         // 코인
void play_jump(void);         // 점프 (Flappy Bird용)
void play_mario_1up(void);    // 1-UP
void play_zelda_secret(void); // 젤다 비밀

void play_mario_die(void); // 마리오 죽음
void play_error(void);     // 에러
void play_powerup(void);   // 파워업

#endif // BUZZER_H