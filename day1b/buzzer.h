#ifndef BUZZER_H
#define BUZZER_H

#include <avr/io.h>
#include <util/delay.h>

#define BUZZER_DDR DDRB
#define BUZZER_PIN PB1
#define BUZZER_PORT PORTB

#define PRESCALER_64 (0 << CS02) | (1 << CS01) | (1 << CS00)

// ====================================================================
// Standard Tone Definitions (8MHz, N=64)
// C4 = 261.63 Hz, D4 = 293.66 Hz, E4 = 329.63 Hz, F4 = 349.23 Hz
// G4 = 392.00 Hz, A4 = 440.00 Hz, B4 = 493.88 Hz
// ====================================================================
#define NOTE_C4_OCR 238 // 도
#define NOTE_D4_OCR 212 // 레
#define NOTE_E4_OCR 189 // 미
#define NOTE_F4_OCR 178 // 파
#define NOTE_G4_OCR 158 // 솔
#define NOTE_A4_OCR 141 // 라
#define NOTE_B4_OCR 126 // 시
#define NOTE_C5_OCR 118 // 높은 도

// --- 4옥타브 (저음/중음) ---
#define NOTE_C4_OCR 238 // 261 Hz (마리오 죽음)
#define NOTE_E4_OCR 189 // 329 Hz (마리오 죽음)
#define NOTE_A4_OCR 141 // 440 Hz (젤다)
#define NOTE_B4_OCR 126 // 493 Hz (마리오 죽음)

// --- 5옥타브 (메인 멜로디) ---
#define NOTE_C5_OCR 118 // 523 Hz (마리오 죽음)
#define NOTE_D5_OCR 105 // 587 Hz (마리오 죽음)
#define NOTE_DS5_OCR 99 // 622 Hz (젤다)
#define NOTE_E5_OCR 94  // 659 Hz (젤다, 마리오 1UP, 마리오 죽음)
#define NOTE_F5_OCR 88  // 698 Hz (마리오 죽음)
#define NOTE_FS5_OCR 83 // 740 Hz (젤다)
#define NOTE_G5_OCR 78  // 784 Hz (젤다, 마리오 1UP)
#define NOTE_GS5_OCR 74 // 830 Hz (젤다)
#define NOTE_B5_OCR 62  // 988 Hz (코인)

// --- 6옥타브 (고음 효과음) ---
#define NOTE_C6_OCR 59 // 1047 Hz (젤다, 마리오 1UP)
#define NOTE_D6_OCR 52 // 1175 Hz (마리오 1UP)
#define NOTE_E6_OCR 46 // 1319 Hz (코인, 마리오 1UP)
#define NOTE_G6_OCR 39 // 1568 Hz (마리오 1UP)

// ====================================================================
// Function Prototypes (음계 재생 함수 추가)
// ====================================================================

void buzzer_init(void);
void buzzer_start_tone(uint8_t ocr_value);
void buzzer_stop_tone(void);

// 도레미파솔라시 음계 재생 함수
void play_note_c(uint16_t duration_ms);
void play_note_d(uint16_t duration_ms);
void play_note_e(uint16_t duration_ms);
void play_note_f(uint16_t duration_ms);
void play_note_g(uint16_t duration_ms);
void play_note_a(uint16_t duration_ms);
void play_note_b(uint16_t duration_ms);
void play_note_c5(uint16_t duration_ms);

void play_coin(void);
void play_error_sound(void);
void play_zelda_secret(void);
void play_mario_1up(void);

#endif // BUZZER_H