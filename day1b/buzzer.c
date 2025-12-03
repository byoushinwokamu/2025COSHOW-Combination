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

// ====================================================================
// 기존 기능 확인 음 재생 함수 (OCR 값 수정 반영)
// ====================================================================

void play_coin(void)
{
  // 1. 첫 번째 음: B5 (시) - 짧고 빠르게
  buzzer_start_tone(NOTE_B5_OCR);

  uint8_t i = 3;
  while (i--) _delay_ms(10); // 약 30ms 유지

  // 2. 두 번째 음: E6 (미) - 바로 이어서 짧게
  // stop 없이 start를 바로 호출하면 소리가 끊기지 않고 주파수만 바뀝니다.
  buzzer_start_tone(NOTE_E6_OCR);

  i = 10;
  while (i--) _delay_ms(10); // 약 100ms 유지

  // 3. 끝
  buzzer_stop_tone();
}

/**
 * @brief 오류/잠금 실패음을 재생합니다. (낮은 띡)
 */
void play_error_sound(void)
{
  // [실패] 비밀번호 오류 "삐-삐-삐-삐" (4회 반복)
  uint8_t tone = 158;

  for (int k = 0; k < 4; k++)
  {
    buzzer_start_tone(tone);
    // 150ms 켜짐
    for (uint8_t i = 0; i < 150; i++) _delay_ms(1);

    buzzer_stop_tone();
    // 100ms 꺼짐
    _delay_ms(100);
  }
}

// [성공] 젤다의 전설 - 시크릿 사운드
void play_zelda_secret(void)
{
  // 멜로디: G5 - F#5 - D#5 - A4 - G#4 - E5 - G#5 - C6
  // 각 음을 약 80~90ms 정도로 빠르게 연주

  uint8_t notes[] = {
      NOTE_G5_OCR, NOTE_FS5_OCR, NOTE_DS5_OCR, NOTE_A4_OCR,
      150 /*G#4대용*/, NOTE_E5_OCR, NOTE_GS5_OCR, NOTE_C6_OCR};

  for (int i = 0; i < 8; i++)
  {
    buzzer_start_tone(notes[i]);

    // 약 90ms 지연
    for (uint8_t k = 0; k < 90; k++) _delay_ms(1);
  }
  buzzer_stop_tone();
}

// [알림] 슈퍼 마리오 1-UP
void play_mario_1up(void)
{
  // E5 - G5 - E6 - C6 - D6 - G6
  uint8_t notes[] = {
      NOTE_E5_OCR, NOTE_G5_OCR, NOTE_E6_OCR,
      NOTE_C6_OCR, NOTE_D6_OCR, NOTE_G6_OCR};

  for (int i = 0; i < 6; i++)
  {
    buzzer_start_tone(notes[i]);
    // 아주 빠르게 (약 80ms)
    for (uint8_t k = 0; k < 80; k++) _delay_ms(1);
  }
  buzzer_stop_tone();
}
