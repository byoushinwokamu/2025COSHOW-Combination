#include "buzzer.h"
#include <avr/io.h>
#include <util/delay.h>

/**
 * @brief Buzzer 모듈을 초기화하고 타이머를 설정합니다.
 */
void buzzer_init(void)
{
  // 1. Buzzer 핀 (PB1)을 출력으로 설정
  BUZZER_DDR |= (1 << BUZZER_PIN);

  // 2. Timer/Counter0 제어 레지스터 A (TCCR0A) 설정
  //    - COM0B[1:0] = 01b: OC0B (PB1) 토글 모드 (Toggle on Compare Match)
  //    - WGM0[1:0]  = 10b: CTC 모드 (TOP = OCR0A)
  TCCR0A = (0 << COM0B1) | (1 << COM0B0) |
           (1 << WGM01) | (0 << WGM00);

  // 3. Timer/Counter0 제어 레지스터 B (TCCR0B) 설정: 타이머 정지 (000b)
  TCCR0B = (0 << WGM02) | (0 << CS02) | (0 << CS01) | (0 << CS00);
}

/**
 * @brief 지정된 OCR 값으로 부저 톤 생성을 시작합니다.
 */
void buzzer_start_tone(uint8_t ocr_value)
{
  // 1. 주파수 설정 (OCR0A 값 변경)
  OCR0A = ocr_value;
  // [추가됨] 카운터를 0으로 리셋하여 소리가 튀는 현상 방지
  TCNT0 = 0;
  // 2. 타이머 시작 (분주비 64로 설정)
  TCCR0B = (TCCR0B & ~((1 << CS02) | (1 << CS01) | (1 << CS00))) | PRESCALER_64;
}

/**
 * @brief 부저 톤 생성을 중지합니다.
 */
void buzzer_stop_tone(void)
{
  // 타이머 정지 (클럭 소스를 000b로 설정)
  TCCR0B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));
  // [추가됨] 부저 핀을 강제로 LOW로 내려서 대기 전류 차단 (발열 방지)
  BUZZER_PORT &= ~(1 << BUZZER_PIN);
}

// ====================================================================
// 도레미파솔라시 음계 재생 함수 (duration_ms 동안 재생)
// ====================================================================

void play_note_c(uint16_t duration_ms)
{
  buzzer_start_tone(NOTE_C4_OCR);
  _delay_ms(duration_ms);
  buzzer_stop_tone();
}

void play_note_d(uint16_t duration_ms)
{
  buzzer_start_tone(NOTE_D4_OCR);
  _delay_ms(duration_ms);
  buzzer_stop_tone();
}

void play_note_e(uint16_t duration_ms)
{
  buzzer_start_tone(NOTE_E4_OCR);
  _delay_ms(duration_ms);
  buzzer_stop_tone();
}

void play_note_f(uint16_t duration_ms)
{
  buzzer_start_tone(NOTE_F4_OCR);
  _delay_ms(duration_ms);
  buzzer_stop_tone();
}

void play_note_g(uint16_t duration_ms)
{
  buzzer_start_tone(NOTE_G4_OCR);
  _delay_ms(duration_ms);
  buzzer_stop_tone();
}

void play_note_a(uint16_t duration_ms)
{
  buzzer_start_tone(NOTE_A4_OCR);
  _delay_ms(duration_ms);
  buzzer_stop_tone();
}

void play_note_b(uint16_t duration_ms)
{
  buzzer_start_tone(NOTE_B4_OCR);
  _delay_ms(duration_ms);
  buzzer_stop_tone();
}

void play_note_c5(uint16_t duration_ms)
{
  buzzer_start_tone(NOTE_C5_OCR);
  _delay_ms(duration_ms);
  buzzer_stop_tone();
}

// 슈퍼 마리오브라더스 게임 오버 BGM
void play_mario_death(void)
{
  // 기본 템포: 90
  uint8_t t = 90;

  // ==========================================
  // Part 1. 인트로 (시 - 파 - 파 - 파)
  // ==========================================

  // 1. 시 (B4)
  buzzer_start_tone(NOTE_B4_OCR);
  for (uint8_t i = 0; i < t; i++) _delay_ms(1);
  buzzer_stop_tone();
  _delay_ms(20);

  // 2. 파 (F5)
  buzzer_start_tone(NOTE_F5_OCR);
  for (uint8_t i = 0; i < t; i++) _delay_ms(1);
  buzzer_stop_tone();

  // 3. 엇박자 쉼표
  for (uint8_t i = 0; i < (t + 10); i++) _delay_ms(1);

  // 4. 파 (F5)
  buzzer_start_tone(NOTE_F5_OCR);
  for (uint8_t i = 0; i < t; i++) _delay_ms(1);
  buzzer_stop_tone();
  _delay_ms(20);

  // 5. 파 (F5)
  buzzer_start_tone(NOTE_F5_OCR);
  for (uint8_t i = 0; i < t; i++) _delay_ms(1);
  buzzer_stop_tone();

  _delay_ms(30);

  // ==========================================
  // Part 2. 중간 하강 (파 - 미 - 레 - 도)
  // ==========================================

  // 파 (F5)
  buzzer_start_tone(NOTE_F5_OCR);
  for (uint8_t i = 0; i < t; i++) _delay_ms(1);
  buzzer_stop_tone();
  _delay_ms(50);

  // 미 (E5)
  buzzer_start_tone(NOTE_E5_OCR);
  for (uint8_t i = 0; i < t; i++) _delay_ms(1);
  buzzer_stop_tone();
  _delay_ms(50);

  // 레 (D5)
  buzzer_start_tone(NOTE_D5_OCR);
  for (uint8_t i = 0; i < t; i++) _delay_ms(1);
  buzzer_stop_tone();
  _delay_ms(50);

  // 도 (C5)
  buzzer_start_tone(NOTE_C5_OCR);
  for (uint8_t i = 0; i < t; i++) _delay_ms(1);
  buzzer_stop_tone();

  _delay_ms(100);

  // ==========================================
  // Part 3. 베이스 (미... 미... 도...)
  // ==========================================

  // 1. 미 (E4)
  buzzer_start_tone(NOTE_E4_OCR);
  for (uint8_t i = 0; i < t; i++) _delay_ms(1);
  buzzer_stop_tone();

  // 2. 엇박자 쉼표
  for (uint8_t i = 0; i < t; i++) _delay_ms(1);

  // 3. 미 (E4)
  buzzer_start_tone(NOTE_E4_OCR);
  for (uint8_t i = 0; i < t; i++) _delay_ms(1);
  buzzer_stop_tone();
  _delay_ms(20);

  // 4. 도 (C4)
  buzzer_start_tone(NOTE_C4_OCR);
  for (uint16_t i = 0; i < (t * 3); i++) _delay_ms(1);
  buzzer_stop_tone();
}