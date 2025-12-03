/*
  [Integrated LED Matrix Driver v4.4]
  Mode 0: Text Scrolling
  Mode 1: Diamond Ripple
  Mode 2: Mario Game (Black BG, Blocks, Coins, Sounds) - UPDATED
  Mode 3: Flappy Bird

  Controls:
  - PC5 (A5): Switch Mode (Toggle 0 -> 1 -> 2 -> 3 -> 0)
  - PC4 (A4): Action (Jump / Flap / Restart)
*/

#define F_CPU 16000000UL
#include "buzzer.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <util/delay.h>

// --- 핀 설정 ---
#define PIN_CLK PB0  // D8
#define PIN_LAT PB1  // D9
#define PIN_OE PB2   // D10
#define BTN_MODE PC5 // A5 - Mode Switch
#define BTN_JUMP PC4 // A4 - Action

// --- 공유 버퍼 (384 Bytes) ---
uint8_t frame_buffer[384];

// ============================================================================
// [DATA] 폰트, LUT, 스프라이트 (PROGMEM)
// ============================================================================

// 1. Font (Text Mode)
const uint8_t font5x7[][5] PROGMEM = {
    {0, 0, 0, 0, 0}, {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00}, {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31}, {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03}, {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x60, 0x60, 0x00, 0x00}, {0x00, 0x36, 0x36, 0x00, 0x00}, {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01}, {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40}, {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46}, {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63}, {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43}};

// 2. Sine LUT
const uint8_t sin_lut[256] PROGMEM = {
    128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173,
    176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215,
    218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244,
    245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255,
    255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246,
    245, 244, 243, 241, 240, 238, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220,
    218, 215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182, 179,
    176, 173, 170, 167, 165, 162, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131,
    128, 124, 121, 118, 115, 112, 109, 106, 103, 100, 97, 93, 90, 88, 85, 82,
    79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40,
    37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11,
    10, 9, 7, 6, 5, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 5, 6, 7, 9,
    10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
    37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
    79, 82, 85, 88, 90, 93, 97, 100, 103, 106, 109, 112, 115, 118, 121, 124};

// 3. Mario Sprites (Mario, Blocks, Coins)
const uint8_t mario_run1[224] PROGMEM = {
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 2, 2, 2, 3, 3, 2, 3, 0, 0, 0, 0, 0, 0, 2, 3, 2, 3, 3, 3, 2, 3, 3, 3, 0, 0, 0, 0, 2, 3, 2, 2, 3, 3, 3, 2, 3, 3, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 2, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 1, 2, 1, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 1, 1, 1, 1, 2, 2, 0, 0, 0, 0, 3, 3, 3, 1, 2, 1, 1, 2, 1, 3, 3, 3, 0, 0, 3, 3, 3, 3, 1, 1, 1, 1, 1, 3, 3, 3, 0, 0, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 3, 3, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0};
const uint8_t mario_run2[224] PROGMEM = {
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 2, 2, 2, 3, 3, 2, 3, 0, 0, 0, 0, 0, 0, 2, 3, 2, 3, 3, 3, 2, 3, 3, 3, 0, 0, 0, 0, 2, 3, 2, 2, 3, 3, 3, 2, 3, 3, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 2, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 1, 2, 1, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 1, 1, 1, 1, 2, 2, 0, 0, 0, 0, 3, 3, 3, 1, 2, 1, 1, 2, 1, 3, 3, 3, 0, 0, 3, 3, 3, 3, 1, 1, 1, 1, 1, 3, 3, 3, 0, 0, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 3, 3, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 2, 2, 2, 2, 0, 0};
const uint8_t mario_jump[224] PROGMEM = {
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 2, 2, 2, 3, 3, 2, 3, 0, 0, 0, 0, 0, 0, 2, 3, 2, 3, 3, 3, 2, 3, 3, 3, 0, 0, 0, 0, 2, 3, 2, 2, 3, 3, 3, 2, 3, 3, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 2, 2, 1, 1, 1, 2, 1, 2, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 0, 0, 3, 3, 3, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0};
const uint8_t block_q[196] PROGMEM = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 4, 4, 4, 4, 5, 5, 5, 5, 4, 4, 5, 5, 5, 4, 5, 5, 5, 5, 4, 5, 5, 5, 4, 4, 5, 5, 5, 4, 5, 5, 5, 5, 4, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 5, 4, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
const uint8_t block_e[196] PROGMEM = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
const uint8_t coin_spr[80] PROGMEM = {
    0, 0, 4, 4, 4, 4, 0, 0, 0, 4, 5, 5, 5, 5, 4, 0, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 7, 7, 5, 5, 4, 4, 5, 5, 7, 7, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4, 0, 4, 5, 5, 5, 5, 4, 0, 0, 0, 4, 4, 4, 4, 0, 0};

const uint8_t JUMP_HEIGHTS[] = {0, 4, 8, 12, 15, 18, 20, 22, 23, 24, 24, 23, 22, 20, 18, 15, 12, 8, 4, 0};
const uint8_t JUMP_LEN = 20;

// 4. Flappy Bird Sprite (5x4)
const uint8_t flappy_sprite[20] PROGMEM = {
    0, 1, 1, 1, 0, 1, 2, 2, 1, 0, 1, 1, 1, 1, 3, 0, 1, 1, 1, 0};

// Messages
const uint8_t msg1[] = {3, 1, 3, 6, 0, 16, 28, 11, 32, 21, 28, 36, 0, 0, 0};
const uint8_t msg2[] = {26, 16, 34, 0, 16, 28, 27, 33, 18, 32, 33, 0, 0, 0};
const uint8_t msg3[] = {33, 18, 14, 26, 13, 0, 16, 28, 26, 15, 22, 27, 14, 33, 22, 28, 27, 0, 0, 0};
const uint8_t msg4[] = {3, 1, 3, 6, 12, 2, 2, 12, 3, 8, 12, 0, 0, 0};
const uint8_t len1 = 15, len2 = 14, len3 = 20, len4 = 14;

// ============================================================================
// [Shared Functions]
// ============================================================================
void init_hardware(void)
{
  DDRD |= 0xFC;
  DDRB |= 0x07;
  DDRC |= 0x0F;
  DDRC &= ~((1 << BTN_MODE) | (1 << BTN_JUMP));
  PORTC |= (1 << BTN_MODE) | (1 << BTN_JUMP); // Pull-up
  PORTB |= (1 << PIN_OE);
  buzzer_init();
}

// ============================================================================
// [MODE 0] Text Scrolling
// ============================================================================
#define TEXT_BAM_LOOP 20
#define TEXT_SCROLL_SPEED 1
#define TEXT_COLOR_SPEED 3

static int16_t txt_s1 = 64, txt_s2 = 64, txt_s3 = 64, txt_s4 = 64;
static uint16_t txt_color_tick = 0;
static uint16_t txt_scroll_timer = 0;

void render_mode_text(uint8_t row)
{
  PORTB |= (1 << PIN_OE);
  PORTC = (PORTC & 0xF0) | (row & 0x0F);
  for (uint8_t plane = 0; plane < 3; plane++)
  {
    uint8_t *ptr = &frame_buffer[plane * 128];
    for (uint8_t i = 0; i < 128; i++)
    {
      PORTB &= ~(1 << PIN_CLK);
      PORTD = (PORTD & 0x03) | *ptr++;
      PORTB |= (1 << PIN_CLK);
    }
    PORTB |= (1 << PIN_LAT);
    PORTB &= ~(1 << PIN_LAT);
    PORTB &= ~(1 << PIN_OE);
    uint16_t loops = TEXT_BAM_LOOP * (1 << plane);
    while (loops--)
    {
      __asm__ __volatile__("nop");
    }
    PORTB |= (1 << PIN_OE);
  }
}

void logic_mode_text(uint8_t row)
{
  uint8_t *p0 = &frame_buffer[0], *p1 = &frame_buffer[128], *p2 = &frame_buffer[256];
  const uint8_t Y_LINE1 = 4, Y_LINE2 = 18, Y_LINE3 = 34, Y_LINE4 = 50;

  static uint8_t rRain[64], gRain[64], bRain[64];
  if (row == 0)
  {
    for (uint8_t x = 0; x < 64; x++)
    {
      uint8_t hue = (uint8_t)(x * 4 + txt_color_tick);
      rRain[x] = pgm_read_byte(&sin_lut[hue]) >> 5;
      gRain[x] = pgm_read_byte(&sin_lut[(uint8_t)(hue + 85)]) >> 5;
      bRain[x] = pgm_read_byte(&sin_lut[(uint8_t)(hue + 170)]) >> 5;
    }
  }

  for (uint8_t i = 0; i < 128; i++)
  {
    uint8_t x = i & 0x3F;
    uint8_t r1 = 0, g1 = 0, b1 = 0, r2 = 0, g2 = 0, b2 = 0;
    uint8_t y_top = (i < 64) ? (row + 32) : row;
    uint8_t y_bot = (i < 64) ? (row + 48) : (row + 16);

    int16_t t_current = (i < 64) ? (-txt_s3 + i) : (-txt_s1 + (i - 64));
    int16_t ch_idx = (t_current >= 0) ? (t_current / 6) : -(((-t_current) + 5) / 6);
    uint8_t col_idx = (uint8_t)(t_current - ch_idx * 6);

    uint8_t pixel_on = 0, line_mode = 0;
    if (y_top >= Y_LINE1 && y_top < Y_LINE1 + 8)
    {
      if (i >= 64 && col_idx < 5 && ch_idx >= 0 && ch_idx < len1)
      {
        uint8_t cc = msg1[ch_idx];
        if (cc > 39) cc = 0;
        if (pgm_read_byte(&font5x7[cc][col_idx]) & (1 << (y_top - Y_LINE1))) pixel_on = 1;
      }
      line_mode = 1;
    }
    else if (y_top >= Y_LINE2 && y_top < Y_LINE2 + 8)
    {
      if (i >= 64 && col_idx < 5 && ch_idx >= 0 && ch_idx < len2)
      {
        uint8_t cc = msg2[ch_idx];
        if (cc > 39) cc = 0;
        if (pgm_read_byte(&font5x7[cc][col_idx]) & (1 << (y_top - Y_LINE2))) pixel_on = 1;
      }
      line_mode = 2;
    }
    else if (y_top >= Y_LINE3 && y_top < Y_LINE3 + 8)
    {
      if (i < 64 && col_idx < 5 && ch_idx >= 0 && ch_idx < len3)
      {
        uint8_t cc = msg3[ch_idx];
        if (cc > 39) cc = 0;
        if (pgm_read_byte(&font5x7[cc][col_idx]) & (1 << (y_top - Y_LINE3))) pixel_on = 1;
      }
      line_mode = 3;
    }
    else if (y_top >= Y_LINE4 && y_top < Y_LINE4 + 8)
    {
      if (i < 64 && col_idx < 5 && ch_idx >= 0 && ch_idx < len4)
      {
        uint8_t cc = msg4[ch_idx];
        if (cc > 39) cc = 0;
        if (pgm_read_byte(&font5x7[cc][col_idx]) & (1 << (y_top - Y_LINE4))) pixel_on = 1;
      }
      line_mode = 4;
    }

    if (pixel_on)
    {
      if (line_mode == 1)
      {
        r1 = rRain[x];
        g1 = gRain[x];
        b1 = bRain[x];
      }
      else if (line_mode == 2)
      {
        r1 = 0;
        g1 = 6;
        b1 = 7;
      }
      else if (line_mode == 3)
      {
        r1 = 7;
        g1 = 7;
        b1 = 0;
      }
      else
      {
        r1 = 7;
        g1 = 7;
        b1 = 7;
      }
    }

    pixel_on = 0;
    line_mode = 0;
    if (y_bot >= Y_LINE1 && y_bot < Y_LINE1 + 8)
    {
      if (i >= 64 && col_idx < 5 && ch_idx >= 0 && ch_idx < len1)
      {
        uint8_t cc = msg1[ch_idx];
        if (cc > 39) cc = 0;
        if (pgm_read_byte(&font5x7[cc][col_idx]) & (1 << (y_bot - Y_LINE1))) pixel_on = 1;
      }
      line_mode = 1;
    }
    else if (y_bot >= Y_LINE2 && y_bot < Y_LINE2 + 8)
    {
      if (i >= 64 && col_idx < 5 && ch_idx >= 0 && ch_idx < len2)
      {
        uint8_t cc = msg2[ch_idx];
        if (cc > 39) cc = 0;
        if (pgm_read_byte(&font5x7[cc][col_idx]) & (1 << (y_bot - Y_LINE2))) pixel_on = 1;
      }
      line_mode = 2;
    }
    else if (y_bot >= Y_LINE3 && y_bot < Y_LINE3 + 8)
    {
      if (i < 64 && col_idx < 5 && ch_idx >= 0 && ch_idx < len3)
      {
        uint8_t cc = msg3[ch_idx];
        if (cc > 39) cc = 0;
        if (pgm_read_byte(&font5x7[cc][col_idx]) & (1 << (y_bot - Y_LINE3))) pixel_on = 1;
      }
      line_mode = 3;
    }
    else if (y_bot >= Y_LINE4 && y_bot < Y_LINE4 + 8)
    {
      if (i < 64 && col_idx < 5 && ch_idx >= 0 && ch_idx < len4)
      {
        uint8_t cc = msg4[ch_idx];
        if (cc > 39) cc = 0;
        if (pgm_read_byte(&font5x7[cc][col_idx]) & (1 << (y_bot - Y_LINE4))) pixel_on = 1;
      }
      line_mode = 4;
    }

    if (pixel_on)
    {
      if (line_mode == 1)
      {
        r2 = rRain[x];
        g2 = gRain[x];
        b2 = bRain[x];
      }
      else if (line_mode == 2)
      {
        r2 = 0;
        g2 = 6;
        b2 = 7;
      }
      else if (line_mode == 3)
      {
        r2 = 7;
        g2 = 7;
        b2 = 0;
      }
      else
      {
        r2 = 7;
        g2 = 7;
        b2 = 7;
      }
    }

    uint8_t val0 = 0, val1 = 0, val2 = 0;
    if (r1 & 1) val0 |= 0x04;
    if (g1 & 1) val0 |= 0x08;
    if (b1 & 1) val0 |= 0x10;
    if (r2 & 1) val0 |= 0x20;
    if (g2 & 1) val0 |= 0x40;
    if (b2 & 1) val0 |= 0x80;
    if (r1 & 2) val1 |= 0x04;
    if (g1 & 2) val1 |= 0x08;
    if (b1 & 2) val1 |= 0x10;
    if (r2 & 2) val1 |= 0x20;
    if (g2 & 2) val1 |= 0x40;
    if (b2 & 2) val1 |= 0x80;
    if (r1 & 4) val2 |= 0x04;
    if (g1 & 4) val2 |= 0x08;
    if (b1 & 4) val2 |= 0x10;
    if (r2 & 4) val2 |= 0x20;
    if (g2 & 4) val2 |= 0x40;
    if (b2 & 4) val2 |= 0x80;
    *p0++ = val0;
    *p1++ = val1;
    *p2++ = val2;
  }
}

void update_mode_text(void)
{
  txt_color_tick += TEXT_COLOR_SPEED;
  txt_scroll_timer++;
  if (txt_scroll_timer > TEXT_SCROLL_SPEED)
  {
    txt_scroll_timer = 0;
    txt_s1--;
    if (txt_s1 < -(len1 * 6)) txt_s1 = 64;
    txt_s2--;
    if (txt_s2 < -(len2 * 6)) txt_s2 = 64;
    txt_s3--;
    if (txt_s3 < -(len3 * 6)) txt_s3 = 64;
    txt_s4--;
    if (txt_s4 < -(len4 * 6)) txt_s4 = 64;
  }
}

// ============================================================================
// [MODE 1] Diamond Ripple
// ============================================================================
#define SPEC_BAM_LOOP 40
#define SPEC_SPEED_STEP 3

static uint16_t spec_t_val = 0;

void render_mode_spectrum(uint8_t row)
{
  PORTB |= (1 << PIN_OE);
  PORTC = (PORTC & 0xF0) | (row & 0x0F);
  for (uint8_t plane = 0; plane < 3; plane++)
  {
    uint8_t *ptr = &frame_buffer[plane * 128];
    for (uint8_t i = 0; i < 128; i++)
    {
      PORTB &= ~(1 << PIN_CLK);
      PORTD = (PORTD & 0x03) | *ptr++;
      PORTB |= (1 << PIN_CLK);
    }
    PORTB |= (1 << PIN_LAT);
    PORTB &= ~(1 << PIN_LAT);
    PORTB &= ~(1 << PIN_OE);
    uint16_t loops = SPEC_BAM_LOOP * (1 << plane);
    while (loops--)
    {
      __asm__ __volatile__("nop");
    }
    PORTB |= (1 << PIN_OE);
  }
}

void logic_mode_spectrum(uint8_t row)
{
  uint8_t r1, g1, b1, r2, g2, b2;
  uint8_t *p0 = &frame_buffer[0], *p1 = &frame_buffer[128], *p2 = &frame_buffer[256];
  const int16_t center_x = 31;
  const int16_t center_y = 31;

  for (uint8_t i = 0; i < 128; i++)
  {
    uint8_t x = i % 64;
    uint8_t logic_y_top, logic_y_bot;
    if (i < 64)
    {
      logic_y_top = row + 32;
      logic_y_bot = row + 48;
    }
    else
    {
      logic_y_top = row;
      logic_y_bot = row + 16;
    }

    uint16_t dist_top = abs((int16_t)x - center_x) + abs((int16_t)logic_y_top - center_y);
    uint16_t dist_bot = abs((int16_t)x - center_x) + abs((int16_t)logic_y_bot - center_y);

    r1 = pgm_read_byte(&sin_lut[(dist_top * 3 + spec_t_val) & 0xFF]) >> 5;
    g1 = pgm_read_byte(&sin_lut[(dist_top * 4 + spec_t_val * 2 + 85) & 0xFF]) >> 5;
    b1 = pgm_read_byte(&sin_lut[(dist_top * 5 + spec_t_val * 3 + 170) & 0xFF]) >> 5;

    r2 = pgm_read_byte(&sin_lut[(dist_bot * 3 + spec_t_val) & 0xFF]) >> 5;
    g2 = pgm_read_byte(&sin_lut[(dist_bot * 4 + spec_t_val * 2 + 85) & 0xFF]) >> 5;
    b2 = pgm_read_byte(&sin_lut[(dist_bot * 5 + spec_t_val * 3 + 170) & 0xFF]) >> 5;

    if (r1 > 7) r1 = 7;
    if (g1 > 7) g1 = 7;
    if (b1 > 7) b1 = 7;
    if (r2 > 7) r2 = 7;
    if (g2 > 7) g2 = 7;
    if (b2 > 7) b2 = 7;

    uint8_t val0 = 0, val1 = 0, val2 = 0;
    if (r1 & 1) val0 |= 0x04;
    if (g1 & 1) val0 |= 0x08;
    if (b1 & 1) val0 |= 0x10;
    if (r2 & 1) val0 |= 0x20;
    if (g2 & 1) val0 |= 0x40;
    if (b2 & 1) val0 |= 0x80;
    if (r1 & 2) val1 |= 0x04;
    if (g1 & 2) val1 |= 0x08;
    if (b1 & 2) val1 |= 0x10;
    if (r2 & 2) val1 |= 0x20;
    if (g2 & 2) val1 |= 0x40;
    if (b2 & 2) val1 |= 0x80;
    if (r1 & 4) val2 |= 0x04;
    if (g1 & 4) val2 |= 0x08;
    if (b1 & 4) val2 |= 0x10;
    if (r2 & 4) val2 |= 0x20;
    if (g2 & 4) val2 |= 0x40;
    if (b2 & 4) val2 |= 0x80;
    *p0++ = val0;
    *p1++ = val1;
    *p2++ = val2;
  }
}

void update_mode_spectrum(void)
{
  spec_t_val += SPEC_SPEED_STEP;
}

// ============================================================================
// [MODE 2] Mario (Black BG, Blocks, Coins)
// ============================================================================
#define MARIO_BASE_TIME 4

static uint16_t mar_scroll_x = 0;
static uint8_t mar_frame_tick = 0;
static uint8_t mar_mario_frame = 0;
static uint8_t mar_jump_state = 0;
static uint8_t mar_jump_idx = 0;
static uint8_t mar_current_y = 0;

// [NEW] Props Variables
static int16_t mar_block_x = 64;
static uint8_t mar_block_hit = 0;
static uint8_t mar_coin_active = 0;
static int16_t mar_coin_x = 0;
static uint8_t mar_coin_y = 0;

void render_mode_mario(uint8_t row)
{
  for (uint8_t plane = 0; plane < 3; plane++)
  {
    PORTB |= (1 << PIN_OE);
    PORTC = (PORTC & 0xF0) | (row & 0x0F);
    uint8_t *ptr = &frame_buffer[plane * 128];
    for (uint8_t col = 0; col < 128; col++)
    {
      PORTD = (PORTD & 0x03) | *ptr++;
      PORTB |= (1 << PIN_CLK);
      PORTB &= ~(1 << PIN_CLK);
    }
    PORTB |= (1 << PIN_LAT);
    PORTB &= ~(1 << PIN_LAT);
    PORTB &= ~(1 << PIN_OE);
    if (plane == 0)
    {
      _delay_us(MARIO_BASE_TIME);
    }
    else if (plane == 1)
    {
      _delay_us(MARIO_BASE_TIME * 2);
    }
    else
    {
      _delay_us(MARIO_BASE_TIME * 4);
    }
  }
  PORTB |= (1 << PIN_OE);
}

// Helper macro for drawing colored pixels
#define DRAW_PX(c, r, g, b) \
  if (c)                    \
  {                         \
    if (c == 1)             \
    {                       \
      *r = 7;               \
      *g = 0;               \
      *b = 0;               \
    }                       \
    else if (c == 2)        \
    {                       \
      *r = 4;               \
      *g = 2;               \
      *b = 0;               \
    }                       \
    else if (c == 3)        \
    {                       \
      *r = 7;               \
      *g = 5;               \
      *b = 3;               \
    }                       \
    else if (c == 4)        \
    {                       \
      *r = 0;               \
      *g = 0;               \
      *b = 0;               \
    }                       \
    else if (c == 5)        \
    {                       \
      *r = 7;               \
      *g = 6;               \
      *b = 0;               \
    }                       \
    else if (c == 6)        \
    {                       \
      *r = 3;               \
      *g = 2;               \
      *b = 0;               \
    }                       \
    else if (c == 7)        \
    {                       \
      *r = 7;               \
      *g = 7;               \
      *b = 7;               \
    }                       \
  }

void logic_mode_mario(uint8_t row)
{
  // Clear buffer (0 initialized)
  for (int i = 0; i < 384; i++) frame_buffer[i] = 0;

  const uint8_t MARIO_X = 23;
  const uint8_t MARIO_BASE_Y = 60;
  int8_t mario_draw_y = MARIO_BASE_Y - mar_current_y - 16;
  const uint8_t BLOCK_Y = 16;

  // Coin Y calculation for rendering (Coin moves up)
  uint8_t render_coin_y = (mar_coin_active) ? (16 - mar_coin_y) : 0;

  for (uint8_t col = 0; col < 128; col++)
  {
    uint8_t vx = col % 64;
    uint8_t y_top = (col < 64) ? (row + 32) : row;
    uint8_t y_bot = (col < 64) ? (row + 48) : (row + 16);

    uint8_t r1 = 0, g1 = 0, b1 = 0;
    uint8_t r2 = 0, g2 = 0, b2 = 0;

    // [Background] Black Sky + Brown Floor (Y>=60)
    if (y_top >= 60)
    {
      r1 = 6;
      g1 = 3;
      b1 = 0;
    }
    if (y_bot >= 60)
    {
      r2 = 6;
      g2 = 3;
      b2 = 0;
    }

    // 1. Coin
    if (mar_coin_active && vx >= mar_coin_x && vx < mar_coin_x + 8)
    {
      if (y_top >= render_coin_y && y_top < render_coin_y + 10)
      {
        DRAW_PX(pgm_read_byte(&coin_spr[(y_top - render_coin_y) * 8 + (vx - mar_coin_x)]), &r1, &g1, &b1);
      }
      if (y_bot >= render_coin_y && y_bot < render_coin_y + 10)
      {
        DRAW_PX(pgm_read_byte(&coin_spr[(y_bot - render_coin_y) * 8 + (vx - mar_coin_x)]), &r2, &g2, &b2);
      }
    }

    // 2. Block
    if (vx >= mar_block_x && vx < mar_block_x + 14)
    {
      const uint8_t *bp = mar_block_hit ? block_e : block_q;
      int16_t bx_offset = vx - mar_block_x;

      if (y_top >= BLOCK_Y && y_top < BLOCK_Y + 14)
      {
        DRAW_PX(pgm_read_byte(&bp[(y_top - BLOCK_Y) * 14 + bx_offset]), &r1, &g1, &b1);
      }
      if (y_bot >= BLOCK_Y && y_bot < BLOCK_Y + 14)
      {
        DRAW_PX(pgm_read_byte(&bp[(y_bot - BLOCK_Y) * 14 + bx_offset]), &r2, &g2, &b2);
      }
    }

    // 3. Mario
    const uint8_t *sp = (mar_current_y > 0) ? mario_jump : ((mar_mario_frame == 0) ? mario_run1 : mario_run2);
    if (vx >= MARIO_X && vx < MARIO_X + 14)
    {
      if (y_top >= mario_draw_y && y_top < mario_draw_y + 16)
      {
        DRAW_PX(pgm_read_byte(&sp[(y_top - mario_draw_y) * 14 + (vx - MARIO_X)]), &r1, &g1, &b1);
      }
      if (y_bot >= mario_draw_y && y_bot < mario_draw_y + 16)
      {
        DRAW_PX(pgm_read_byte(&sp[(y_bot - mario_draw_y) * 14 + (vx - MARIO_X)]), &r2, &g2, &b2);
      }
    }

    uint8_t pd0 = 0, pd1 = 0, pd2 = 0;
    if (r1 & 1) pd0 |= 0x04;
    if (g1 & 1) pd0 |= 0x08;
    if (b1 & 1) pd0 |= 0x10;
    if (r2 & 1) pd0 |= 0x20;
    if (g2 & 1) pd0 |= 0x40;
    if (b2 & 1) pd0 |= 0x80;
    frame_buffer[col] = pd0;
    if (r1 & 2) pd1 |= 0x04;
    if (g1 & 2) pd1 |= 0x08;
    if (b1 & 2) pd1 |= 0x10;
    if (r2 & 2) pd1 |= 0x20;
    if (g2 & 2) pd1 |= 0x40;
    if (b2 & 2) pd1 |= 0x80;
    frame_buffer[128 + col] = pd1;
    if (r1 & 4) pd2 |= 0x04;
    if (g1 & 4) pd2 |= 0x08;
    if (b1 & 4) pd2 |= 0x10;
    if (r2 & 4) pd2 |= 0x20;
    if (g2 & 4) pd2 |= 0x40;
    if (b2 & 4) pd2 |= 0x80;
    frame_buffer[256 + col] = pd2;
  }
}

void update_mode_mario(void)
{
  mar_frame_tick++;
  if (mar_frame_tick > 3)
  {
    mar_frame_tick = 0;
    mar_scroll_x++;

    // Mario Jump & Logic
    if (mar_jump_state == 0)
    {
      if ((mar_scroll_x % 4) == 0) mar_mario_frame = !mar_mario_frame;
      mar_current_y = 0;

      // Jump Button
      if (!(PINC & (1 << BTN_JUMP)))
      {
        mar_jump_state = 1;
        mar_jump_idx = 0;
        play_jump_start(); // [SOUND] Jump Sound
      }
    }
    else
    {
      // [Collision Detection]
      if (!mar_block_hit && mar_jump_idx < JUMP_LEN / 2)
      {
        int8_t mario_head_y = 60 - mar_current_y - 16;

        // Check Overlap with Block (Block is at Y=16, Height=14)
        if (mar_block_x < 36 && (mar_block_x + 14) > 24)
        {
          if (mario_head_y <= 30 && mario_head_y > 25)
          {
            mar_block_hit = 1;
            mar_coin_active = 1;
            mar_coin_x = mar_block_x + 3;
            mar_coin_y = 0;
            play_coin_start(); // [SOUND] Coin Sound

            // Bounce down
            mar_jump_idx = JUMP_LEN - mar_jump_idx;
          }
        }
      }

      if (mar_jump_idx < JUMP_LEN)
      {
        mar_current_y = JUMP_HEIGHTS[mar_jump_idx];
        mar_jump_idx++;
      }
      else
      {
        mar_jump_state = 0;
        mar_current_y = 0;
      }
    }

    // Block Scrolling
    mar_block_x--;
    if (mar_block_x < -14)
    {
      mar_block_x = 64 + (rand() % 32);
      mar_block_hit = 0;
    }

    // Coin Animation
    if (mar_coin_active)
    {
      mar_coin_x--;
      mar_coin_y += 3;
      if (mar_coin_y > 20) mar_coin_active = 0;
    }
  }
}

// ============================================================================
// [MODE 3] Flappy Bird (Full 64-Row Support)
// ============================================================================
#define FLAPPY_TICK_RATE 3
#define BIRD_X 10
#define PIPE_W 6
#define PIPE_GAP_H 10
// 패널 전체 높이는 64라고 가정

static int8_t bird_y = 32; // 중앙 시작
static int8_t bird_vel = 0;
static uint8_t pipes_x[2] = {64, 96};
static uint8_t pipes_gap_y[2] = {32, 40}; // 파이프 위치도 아래쪽 대응
static uint8_t flap_score = 0;
static uint8_t flap_game_over = 0;
static uint8_t flap_tick = 0;
static uint8_t flap_btn_prev = 0;

// [픽셀 계산]
static inline uint8_t get_flappy_pixel(uint8_t x, uint8_t y)
{
  // 1. Game Over 시 점수판 표시 (최우선 순위)
  if (flap_game_over)
  {
    const uint8_t SCORE_Y = 20;
    const uint8_t SCORE_X_CENTER = 32;

    uint8_t tens = (flap_score / 10) % 10;
    uint8_t ones = flap_score % 10;
    uint8_t idx_tens = tens + 1;
    uint8_t idx_ones = ones + 1;

    if (y >= SCORE_Y && y < SCORE_Y + 7)
    {
      if (x >= (SCORE_X_CENTER - 6) && x < (SCORE_X_CENTER - 1))
      {
        uint8_t row = y - SCORE_Y;
        uint8_t col = x - (SCORE_X_CENTER - 6);
        if (pgm_read_byte(&font5x7[idx_tens][col]) & (1 << row)) return 255; // White
      }
    }

    if (y >= SCORE_Y && y < SCORE_Y + 7)
    {
      if (x >= SCORE_X_CENTER && x < SCORE_X_CENTER + 5)
      {
        uint8_t row = y - SCORE_Y;
        uint8_t col = x - SCORE_X_CENTER;
        if (pgm_read_byte(&font5x7[idx_ones][col]) & (1 << row)) return 255; // White
      }
    }
  }

  // 2. 파이프 그리기
  for (uint8_t i = 0; i < 2; i++)
  {
    if (x >= pipes_x[i] && x < pipes_x[i] + PIPE_W)
    {
      if (y < pipes_gap_y[i] - (PIPE_GAP_H / 2) || y > pipes_gap_y[i] + (PIPE_GAP_H / 2))
      {
        return 2; // Green
      }
    }
  }
  // 3. 새 그리기
  if (x >= BIRD_X && x < BIRD_X + 5 && y >= bird_y && y < bird_y + 4)
  {
    uint8_t px = x - BIRD_X;
    uint8_t py = y - bird_y;
    return pgm_read_byte(&flappy_sprite[py * 5 + px]);
  }

  // 4. (선택사항) 게임 플레이 중 현재 점수 막대 (바닥에 작게 표시 유지)
  if (!flap_game_over && y == 63 && x < (flap_score % 64)) return 3; // Red Bar (Optional)

  return 0; // Black
}

void render_mode_flappy(uint8_t row)
{
  render_mode_mario(row);
}

void logic_mode_flappy(uint8_t row)
{
  for (int i = 0; i < 384; i++) frame_buffer[i] = 0;
  for (uint8_t col = 0; col < 128; col++)
  {
    uint8_t vx = col % 64;
    // 논리적 Y좌표 매핑 (64x32 Dual Chained -> 64x64 Logical)
    uint8_t y_top = (col < 64) ? (row + 32) : row;
    uint8_t y_bot = (col < 64) ? (row + 48) : (row + 16);

    uint8_t r1 = 0, g1 = 0, b1 = 0, r2 = 0, g2 = 0, b2 = 0;

    uint8_t c1 = get_flappy_pixel(vx, y_top);
    if (c1 == 1)
    {
      r1 = 7;
      g1 = 7;
      b1 = 0;
    }
    else if (c1 == 2)
    {
      r1 = 0;
      g1 = 7;
      b1 = 0;
    }
    else if (c1 == 3)
    {
      r1 = 7;
      g1 = 2;
      b1 = 0;
    }
    else if (c1 == 255)
    {
      r1 = 7;
      g1 = 7;
      b1 = 7;
    }

    uint8_t c2 = get_flappy_pixel(vx, y_bot);
    if (c2 == 1)
    {
      r2 = 7;
      g2 = 7;
      b2 = 0;
    }
    else if (c2 == 2)
    {
      r2 = 0;
      g2 = 7;
      b2 = 0;
    }
    else if (c2 == 3)
    {
      r2 = 7;
      g2 = 2;
      b2 = 0;
    }
    else if (c2 == 255)
    {
      r2 = 7;
      g2 = 7;
      b2 = 7;
    }

    uint8_t pd0 = 0, pd1 = 0, pd2 = 0;
    if (r1 & 1) pd0 |= 0x04;
    if (g1 & 1) pd0 |= 0x08;
    if (b1 & 1) pd0 |= 0x10;
    if (r2 & 1) pd0 |= 0x20;
    if (g2 & 1) pd0 |= 0x40;
    if (b2 & 1) pd0 |= 0x80;
    frame_buffer[col] = pd0;
    if (r1 & 2) pd1 |= 0x04;
    if (g1 & 2) pd1 |= 0x08;
    if (b1 & 2) pd1 |= 0x10;
    if (r2 & 2) pd1 |= 0x20;
    if (g2 & 2) pd1 |= 0x40;
    if (b2 & 2) pd1 |= 0x80;
    frame_buffer[128 + col] = pd1;
    if (r1 & 4) pd2 |= 0x04;
    if (g1 & 4) pd2 |= 0x08;
    if (b1 & 4) pd2 |= 0x10;
    if (r2 & 4) pd2 |= 0x20;
    if (g2 & 4) pd2 |= 0x40;
    if (b2 & 4) pd2 |= 0x80;
    frame_buffer[256 + col] = pd2;
  }
}

void update_mode_flappy(void)
{
  uint8_t btn_pressed = !(PINC & (1 << BTN_JUMP));
  uint8_t btn_down = (btn_pressed && !flap_btn_prev);
  flap_btn_prev = btn_pressed;

  if (flap_game_over)
  {
    if (btn_down)
    {
      bird_y = 32;
      bird_vel = 0;
      flap_score = 0;
      pipes_x[0] = 64;
      pipes_x[1] = 96;
      flap_game_over = 0;
      play_coin_start();
    }
    return;
  }
  if (btn_down)
  {
    bird_vel = -3;
    play_coin_start();
  }

  flap_tick++;
  if (flap_tick > FLAPPY_TICK_RATE)
  {
    flap_tick = 0;
    bird_vel++;
    if (bird_vel > 2) bird_vel = 2;
    bird_y += bird_vel;

    for (int i = 0; i < 2; i++)
    {
      pipes_x[i]--;
      if (pipes_x[i] == 255)
      {
        pipes_x[i] = 63;
        pipes_gap_y[i] = (rand() % 40) + 12;
        flap_score++;
      }
    }

    if (bird_y < 0 || bird_y > 60)
    {
      flap_game_over = 1;
      play_zelda_secret();
    }

    for (int i = 0; i < 2; i++)
    {
      if ((BIRD_X + 4 >= pipes_x[i]) && (BIRD_X <= pipes_x[i] + PIPE_W))
      {
        if (bird_y < pipes_gap_y[i] - (PIPE_GAP_H / 2) || bird_y + 3 > pipes_gap_y[i] + (PIPE_GAP_H / 2))
        {
          flap_game_over = 1;
          play_zelda_secret();
        }
      }
    }
  }
}

// ============================================================================
// MAIN
// ============================================================================
int main(void)
{
  init_hardware();
  sei();

  uint8_t mode = 0;
  uint8_t btn_pressed = 0;
  uint8_t prev_mode = 0xFF;

  while (1)
  {
    if (!(PINC & (1 << BTN_MODE)))
    {
      if (!btn_pressed)
      {
        btn_pressed = 1;
        mode++;
        if (mode > 3) mode = 0;
      }
    }
    else
    {
      btn_pressed = 0;
    }

    if (mode != prev_mode)
    {
      switch (mode)
      {
      case 0:
        play_zelda_secret();
        break;
      case 1:
        play_mario_1up();
        break;
      case 2:
        play_powerup();
        break; // Game Start Sound
      case 3:
        play_coin_start();
        break;
      }
      prev_mode = mode;
    }

    for (uint8_t row = 0; row < 16; row++)
    {
      switch (mode)
      {
      case 0:
        logic_mode_text(row);
        render_mode_text(row);
        break;
      case 1:
        logic_mode_spectrum(row);
        render_mode_spectrum(row);
        break;
      case 2:
        logic_mode_mario(row);
        render_mode_mario(row);
        break;
      case 3:
        logic_mode_flappy(row);
        render_mode_flappy(row);
        break;
      }
    }

    switch (mode)
    {
    case 0:
      update_mode_text();
      break;
    case 1:
      update_mode_spectrum();
      break;
    case 2:
      update_mode_mario();
      break;
    case 3:
      update_mode_flappy();
      break;
    }
    buzzer_update();
  }
  return 0;
}