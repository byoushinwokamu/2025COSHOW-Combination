#ifndef _THIRDD_H_
#define _THIRDD_H_

#include <stdint.h>

#define SCREEN_W 64
#define SCREEN_H 64

#ifndef _ROW_T_
#define _ROW_T_
typedef struct
{
  uint8_t r[64];
  uint8_t g[64];
  uint8_t b[64];
} row_t;
#endif

extern row_t fb[SCREEN_H]; // frame buffer
void render_cube_frame(float angleX, float angleY);
void clear_framebuffer(void);

#endif