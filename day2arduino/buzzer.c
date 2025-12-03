/*
 * buzzer.c
 * 부저 동작 구현부 (타이머 인터럽트 포함)
 */

#define F_CPU 16000000UL // delay 함수를 위해 필요

#include "buzzer.h"
#include <avr/interrupt.h>
#include <util/delay.h>

// 타이머 작동 상태 플래그 (이 파일 내부에서만 쓰므로 static)
static volatile uint8_t is_buzzer_on = 0;

// Non-blocking 부저 상태 관리
// State: 0=idle, 1-2=coin notes, 3-5=jump notes
static uint8_t buzzer_state = 0;
static uint16_t buzzer_timer = 0;

/**
 * @brief 타이머0 비교 일치 인터럽트 (TIMER0 COMPA)
 * D12 핀은 하드웨어 PWM 핀이 아니므로 인터럽트로 토글합니다.
 */
ISR(TIMER0_COMPA_vect)
{
  if (is_buzzer_on)
  {
    BUZZER_PORT ^= (1 << BUZZER_PIN); // 핀 상태 반전
  }
}

void buzzer_init(void)
{
  // 1. 핀 설정
  BUZZER_DDR |= (1 << BUZZER_PIN);
  BUZZER_PORT &= ~(1 << BUZZER_PIN);

  // 2. 타이머0 설정 (CTC 모드)
  TCCR0A = (1 << WGM01);
  TCCR0B = 0; // 정지 상태

  // 3. 인터럽트 허용
  TIMSK0 |= (1 << OCIE0A);

  // 전역 인터럽트 활성화 (sei는 main에서 호출해도 되지만 여기서 보장)
  sei();
}

void buzzer_tone(uint8_t ocr_value)
{
  OCR0A = ocr_value; // 주파수 설정
  TCNT0 = 0;         // 카운터 초기화
  is_buzzer_on = 1;  // 소리 켜기 플래그

  // 타이머 시작 (Prescaler 64: CS01=1, CS00=1)
  TCCR0B = (TCCR0B & ~0x07) | ((1 << CS01) | (1 << CS00));
}

void buzzer_stop(void)
{
  is_buzzer_on = 0; // 소리 끄기 플래그

  // 타이머 정지
  TCCR0B &= ~0x07;

  // 핀을 확실히 LOW로
  BUZZER_PORT &= ~(1 << BUZZER_PIN);
}

// ============================================================================
// 효과음 구현
// ============================================================================

// Non-blocking 코인 사운드 (상태 머신)
void play_coin_start(void)
{
  buzzer_state = 1;
  buzzer_timer = 0;
  buzzer_tone(NOTE_B5);
}

// Non-blocking 점프 사운드 (상태 머신)
void play_jump_start(void)
{
  buzzer_state = 3;
  buzzer_timer = 0;
  buzzer_tone(NOTE_C6);
}

void buzzer_update(void)
{
  if (buzzer_state == 0) return; // idle

  buzzer_timer++;

  // Coin sound (states 1-2)
  if (buzzer_state == 1)
  {
    if (buzzer_timer >= 5)
    {
      buzzer_tone(NOTE_E6);
      buzzer_state = 2;
      buzzer_timer = 0;
    }
  }
  else if (buzzer_state == 2)
  {
    if (buzzer_timer >= 24)
    {
      buzzer_stop();
      buzzer_state = 0;
      buzzer_timer = 0;
    }
  }
  // Jump sound (states 3-5)
  else if (buzzer_state == 3)
  {
    if (buzzer_timer >= 3)
    { // C6 for 50ms (~3 frames)
      buzzer_tone(NOTE_E6);
      buzzer_state = 4;
      buzzer_timer = 0;
    }
  }
  else if (buzzer_state == 4)
  {
    if (buzzer_timer >= 3)
    { // E6 for 50ms
      buzzer_tone(NOTE_G6);
      buzzer_state = 5;
      buzzer_timer = 0;
    }
  }
  else if (buzzer_state == 5)
  {
    if (buzzer_timer >= 3)
    { // G6 for 50ms
      buzzer_stop();
      buzzer_state = 0;
      buzzer_timer = 0;
    }
  }
}

// Blocking 버전 (기존 호환성)
void play_coin(void)
{
  buzzer_tone(NOTE_B5);
  _delay_ms(80);
  buzzer_tone(NOTE_E6);
  _delay_ms(400);
  buzzer_stop();
}

void play_mario_1up(void)
{
  uint8_t melody[] = {NOTE_E5, NOTE_G5, NOTE_E6, NOTE_C6, NOTE_D6, NOTE_G6};
  for (int i = 0; i < 6; i++)
  {
    buzzer_tone(melody[i]);
    _delay_ms(80);
  }
  buzzer_stop();
}

void play_zelda_secret(void)
{
  uint8_t notes[] = {NOTE_G5, NOTE_FS6, NOTE_D5, NOTE_A4, NOTE_G4, NOTE_E5, NOTE_GS5, NOTE_C6};
  for (int i = 0; i < 8; i++)
  {
    buzzer_tone(notes[i]);
    _delay_ms(90);
  }
  buzzer_stop();
}

void play_mario_die(void)
{
  buzzer_tone(NOTE_B5);
  _delay_ms(150);
  buzzer_tone(NOTE_F5);
  _delay_ms(150);
  buzzer_stop();
  _delay_ms(50);
  buzzer_tone(NOTE_F5);
  _delay_ms(150);
  buzzer_tone(NOTE_E5);
  _delay_ms(150);
  buzzer_tone(NOTE_D5);
  _delay_ms(150);
  buzzer_tone(NOTE_C5);
  _delay_ms(150);
  buzzer_stop();
}

void play_error(void)
{
  for (int i = 0; i < 3; i++)
  {
    buzzer_tone(NOTE_G5);
    _delay_ms(100);
    buzzer_stop();
    _delay_ms(100);
  }
}

void play_powerup(void)
{
  buzzer_tone(NOTE_G4);
  _delay_ms(100);
  buzzer_tone(NOTE_B4);
  _delay_ms(100);
  buzzer_tone(NOTE_D5);
  _delay_ms(100);
  buzzer_tone(NOTE_G5);
  _delay_ms(100);
  buzzer_tone(NOTE_B5);
  _delay_ms(100);

  buzzer_tone(NOTE_GS5);
  _delay_ms(100);
  buzzer_tone(NOTE_C6);
  _delay_ms(100);
  buzzer_tone(NOTE_D6);
  _delay_ms(100);

  buzzer_stop();
}

void play_jump(void)
{
  buzzer_tone(NOTE_C6);
  _delay_ms(50);
  buzzer_tone(NOTE_E6);
  _delay_ms(50);
  buzzer_tone(NOTE_G6);
  _delay_ms(50);
  buzzer_stop();
}