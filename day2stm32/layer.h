#ifndef _LAYER_H_
#define _LAYER_H_

#include <math.h>
#include <stdint.h>

#define MAX_LAYER 64

#define POSMIN 3
#define POSMAX 60

#ifndef _LAYER_T_
#define _LAYER_T_
typedef struct
{
  uint8_t x, y;
  int8_t dx, dy;
  uint8_t r[7];
  uint8_t g[7];
  uint8_t b[7];
} layer_t;
#endif

#ifndef _ROW_T_
#define _ROW_T_
typedef struct
{
  uint8_t r[64];
  uint8_t g[64];
  uint8_t b[64];
} row_t;
#endif

// extern layer_t layers[MAX_LAYER];
void layer_clear(void);
uint64_t layer_move(void); // 레이어 이동 반영 후 업데이트된 row를 64비트 형태로 반환
row_t layer_capture_row(int r);
void layer_add(layer_t l);
void layer_add_random(void);

#endif