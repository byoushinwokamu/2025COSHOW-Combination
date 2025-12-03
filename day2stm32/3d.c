#include "3d.h"
#include <math.h>

row_t fb[SCREEN_H]; // frame buffer

//==================== 3D/2D 벡터 정의 ====================//

typedef struct
{
  float x, y, z;
} vec3f_t;

typedef struct
{
  int16_t x, y; // screen 좌표
  float z;      // 깊이(카메라 공간 z, painter용)
} vertex_proj_t;

//==================== 큐브 정점/면 정의 ====================//

// -1 ~ 1 범위의 정규화된 큐브
static const vec3f_t cube_vertices[8] = {
    {-1.0f, -1.0f, -1.0f},
    {1.0f, -1.0f, -1.0f},
    {1.0f, 1.0f, -1.0f},
    {-1.0f, 1.0f, -1.0f},

    {-1.0f, -1.0f, 1.0f},
    {1.0f, -1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
    {-1.0f, 1.0f, 1.0f}};

typedef struct
{
  uint8_t idx[4];  // 면을 구성하는 정점 인덱스 4개
  uint8_t r, g, b; // 면 색상
} face_t;

// 앞/뒤/좌/우/위/아래 6면
static const face_t cube_faces[6] = {
    {{0, 1, 2, 3}, 255, 0, 0},   // 앞   - 빨강
    {{4, 5, 6, 7}, 0, 255, 0},   // 뒤   - 초록
    {{0, 3, 7, 4}, 0, 0, 255},   // 왼   - 파랑
    {{1, 2, 6, 5}, 255, 255, 0}, // 오른 - 노랑
    {{3, 2, 6, 7}, 0, 255, 255}, // 위   - 시안
    {{0, 1, 5, 4}, 255, 0, 255}  // 아래 - 마젠타
                                 // {{0, 1, 2, 3}, 240, 120, 120}, // 앞면 - 소프트 레드
                                 // {{4, 5, 6, 7}, 120, 220, 140}, // 뒷면 - 민트 그린
                                 // {{0, 3, 7, 4}, 120, 150, 240}, // 왼쪽 - 소프트 블루
                                 // {{1, 2, 6, 5}, 250, 200, 120}, // 오른쪽 - 웜 오렌지
                                 // {{3, 2, 6, 7}, 180, 220, 250}, // 위 - 스카이 블루
                                 // {{0, 1, 5, 4}, 210, 150, 230}  // 아래 - 퍼플
};

// 큐브 각 면의 노멀(모델 공간)
static const vec3f_t face_normals_model[6] = {
    {0.0f, 0.0f, -1.0f}, // 앞
    {0.0f, 0.0f, 1.0f},  // 뒤
    {-1.0f, 0.0f, 0.0f}, // 왼
    {1.0f, 0.0f, 0.0f},  // 오
    {0.0f, 1.0f, 0.0f},  // 위
    {0.0f, -1.0f, 0.0f}, // 아래
};

//==================== 픽셀/프레임 버퍼 유틸 ====================//

void clear_framebuffer(void)
{
  for (int y = 0; y < SCREEN_H; y++)
  {
    for (int x = 0; x < SCREEN_W; x++)
    {
      fb[y].r[x] = 0;
      fb[y].g[x] = 0;
      fb[y].b[x] = 0;
    }
  }
}

static inline void put_pixel(int x, int y,
                             uint8_t r, uint8_t g, uint8_t b)
{
  if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
  fb[y].r[x] = r;
  fb[y].g[x] = g;
  fb[y].b[x] = b;
}

//==================== 삼각형 채우기 ====================//

typedef struct
{
  int16_t x, y;
} vec2i_t;

static inline int edge_function(vec2i_t a, vec2i_t b, vec2i_t c)
{
  // (c - a) x (b - a)
  return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

static inline int min3(int a, int b, int c)
{
  int m = (a < b) ? a : b;
  return (m < c) ? m : c;
}

static inline int max3(int a, int b, int c)
{
  int m = (a > b) ? a : b;
  return (m > c) ? m : c;
}

static void fill_triangle(vec2i_t v0, vec2i_t v1, vec2i_t v2,
                          uint8_t r, uint8_t g, uint8_t b)
{
  int minX = min3(v0.x, v1.x, v2.x);
  int maxX = max3(v0.x, v1.x, v2.x);
  int minY = min3(v0.y, v1.y, v2.y);
  int maxY = max3(v0.y, v1.y, v2.y);

  if (minX < 0) minX = 0;
  if (minY < 0) minY = 0;
  if (maxX >= SCREEN_W) maxX = SCREEN_W - 1;
  if (maxY >= SCREEN_H) maxY = SCREEN_H - 1;

  int area = edge_function(v0, v1, v2);
  if (area == 0) return; // degenerate triangle

  for (int y = minY; y <= maxY; y++)
  {
    for (int x = minX; x <= maxX; x++)
    {
      vec2i_t p = {(int16_t)x, (int16_t)y};

      int w0 = edge_function(v1, v2, p);
      int w1 = edge_function(v2, v0, p);
      int w2 = edge_function(v0, v1, p);

      // 모두 같은 부호이면 inside
      if ((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
          (w0 <= 0 && w1 <= 0 && w2 <= 0))
      {
        put_pixel(x, y, r, g, b);
      }
    }
  }
}

// quad(4점 면)을 삼각형 2개로 나눠서 채우기
static void fill_face(const vertex_proj_t *proj,
                      const face_t *face,
                      uint8_t r, uint8_t g, uint8_t b)
{
  const vertex_proj_t *v0 = &proj[face->idx[0]];
  const vertex_proj_t *v1 = &proj[face->idx[1]];
  const vertex_proj_t *v2 = &proj[face->idx[2]];
  const vertex_proj_t *v3 = &proj[face->idx[3]];

  vec2i_t p0 = {v0->x, v0->y};
  vec2i_t p1 = {v1->x, v1->y};
  vec2i_t p2 = {v2->x, v2->y};
  vec2i_t p3 = {v3->x, v3->y};

  fill_triangle(p0, p1, p2, r, g, b);
  fill_triangle(p0, p2, p3, r, g, b);
}

//==================== 깊이 정렬(Painter) ====================//

typedef struct
{
  uint8_t face_index;
  float depth; // 이 면의 평균 z
} face_order_t;

static void sort_faces_by_depth(face_order_t *order, int count)
{
  // 면 6개라 그냥 버블 정렬로 충분
  for (int i = 0; i < count - 1; i++)
  {
    for (int j = 0; j < count - 1 - i; j++)
    {
      // 더 먼(더 큰 z) 면을 앞쪽에 배치
      if (order[j].depth < order[j + 1].depth)
      {
        face_order_t tmp = order[j];
        order[j] = order[j + 1];
        order[j + 1] = tmp;
      }
    }
  }
}

//==================== 회전 + 투영 ====================//
// 0~1 float → 3비트 단계(0~7) → 다시 0~255 근사값
static inline uint8_t quantize_3bit(float c)
{
  if (c < 0.0f) c = 0.0f;
  if (c > 1.0f) c = 1.0f;

  int level = (int)(c * 7.0f + 0.5f); // 0~7

  // 완전 꺼진 면은 너무 칙칙하니 1단계 이상으로 올려서 전체적으로 화사하게
  if (level < 1) level = 1;
  if (level > 7) level = 7;

  return (uint8_t)((level * 255) / 7);
}

void render_cube_frame(float angleX, float angleY)
{
  vertex_proj_t proj[8];

  // ===== 1. 회전행렬 준비 (Y -> X) =====
  float cx = cosf(angleX);
  float sx = sinf(angleX);
  float cy = cosf(angleY);
  float sy = sinf(angleY);

  float camera_z = 3.5f; // 카메라와 큐브 거리
  float scale = 33.0f;   // 확대 배율

  // ===== 2. 정점 회전 + 투영 =====
  for (int i = 0; i < 8; i++)
  {
    vec3f_t v = cube_vertices[i];

    // Y축 회전
    float x1 = v.x * cy + v.z * sy;
    float z1 = -v.x * sy + v.z * cy;
    float y1 = v.y;

    // X축 회전
    float y2 = y1 * cx - z1 * sx;
    float z2 = y1 * sx + z1 * cx;
    float x2 = x1;

    float z_cam = z2 + camera_z;
    float invz = 1.0f / z_cam;

    float xp = x2 * invz * scale;
    float yp = y2 * invz * scale;

    int16_t sx_i = (int16_t)(SCREEN_W / 2 + xp);
    int16_t sy_i = (int16_t)(SCREEN_H / 2 - yp);

    proj[i].x = sx_i;
    proj[i].y = sy_i;
    proj[i].z = z_cam;
  }

  // ===== 3. 면 깊이 계산 (Painter용) =====
  face_order_t order[6];
  for (int i = 0; i < 6; i++)
  {
    const face_t *f = &cube_faces[i];
    float zsum = 0.0f;
    for (int k = 0; k < 4; k++)
    {
      zsum += proj[f->idx[k]].z;
    }
    order[i].face_index = (uint8_t)i;
    order[i].depth = zsum * 0.25f;
  }

  // ===== 4. 회전된 노멀(카메라 공간) 계산 =====
  vec3f_t face_normals_cam[6];

  for (int i = 0; i < 6; i++)
  {
    vec3f_t n = face_normals_model[i];

    // Y축 회전
    float x1 = n.x * cy + n.z * sy;
    float z1 = -n.x * sy + n.z * cy;
    float y1 = n.y;

    // X축 회전
    float y2 = y1 * cx - z1 * sx;
    float z2 = y1 * sx + z1 * cx;
    float x2 = x1;

    face_normals_cam[i].x = x2;
    face_normals_cam[i].y = y2;
    face_normals_cam[i].z = z2;
  }

  // ===== 5. 조명 세팅 =====
  // 위 + 약간 오른쪽에서 부드럽게 비추는 라이트
  vec3f_t light_dir = {0.4f, 0.8f, -0.4f};

  float len = sqrtf(light_dir.x * light_dir.x +
                    light_dir.y * light_dir.y +
                    light_dir.z * light_dir.z);
  light_dir.x /= len;
  light_dir.y /= len;
  light_dir.z /= len;

  // 카메라는 z-축 뒤에서 보는 것으로 가정
  vec3f_t view_dir = {0.0f, 0.0f, -1.0f};

  // 기본 면 색 팔레트 (0~1 범위), 3비트 스텝 근처의 파스텔 느낌
  //   대략 (0~7 단계를 0~1로 나눈 값들)
  const float base_colors[6][3] = {
      {6.0f / 7.0f, 3.0f / 7.0f, 3.0f / 7.0f}, // 앞   - 따뜻한 핑크
      {3.0f / 7.0f, 5.0f / 7.0f, 3.0f / 7.0f}, // 뒤   - 소프트 그린
      {3.0f / 7.0f, 4.0f / 7.0f, 6.0f / 7.0f}, // 왼   - 소프트 블루
      {6.0f / 7.0f, 5.0f / 7.0f, 3.0f / 7.0f}, // 오른 - 크림 오렌지
      {4.0f / 7.0f, 5.0f / 7.0f, 6.0f / 7.0f}, // 위   - 스카이 톤
      {5.0f / 7.0f, 4.0f / 7.0f, 6.0f / 7.0f}  // 아래 - 연보라
  };

  uint8_t shaded_r[6], shaded_g[6], shaded_b[6];

  // ===== 6. 각 면에 대해 부드러운 라이팅 계산 =====
  for (int i = 0; i < 6; i++)
  {
    vec3f_t n = face_normals_cam[i];

    // 노멀 정규화
    float nlen = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
    if (nlen > 0.0f)
    {
      n.x /= nlen;
      n.y /= nlen;
      n.z /= nlen;
    }

    // Lambert diffuse: n · L
    float ndotl = n.x * light_dir.x +
                  n.y * light_dir.y +
                  n.z * light_dir.z;
    if (ndotl < 0.0f) ndotl = 0.0f;

    // 부드러운 느낌을 위해:
    // - ambient를 꽤 크게
    // - diffuse 비중은 작게
    // - specular는 아예 빼버림
    float ambient = 0.60f;         // 기본 밝기
    float diffuse = 0.35f * ndotl; // 너무 과하지 않게

    float k = ambient + diffuse;

    // 너무 어둡거나 너무 밝은 극단 방지 → 은은한 변화
    if (k < 0.55f) k = 0.55f;
    if (k > 1.00f) k = 1.00f;

    float br = base_colors[i][0] * k;
    float bg = base_colors[i][1] * k;
    float bb = base_colors[i][2] * k;

    // 3비트 단계에 맞춰 양자화 (0~1 → 0~7 → 0~255)
    shaded_r[i] = quantize_3bit(br);
    shaded_g[i] = quantize_3bit(bg);
    shaded_b[i] = quantize_3bit(bb);
  }

  // ===== 7. 깊이 순서대로 Painter 렌더링 =====
  sort_faces_by_depth(order, 6);

  for (int i = 0; i < 6; i++)
  {
    int idx = order[i].face_index;
    fill_face(proj, &cube_faces[idx],
              shaded_r[idx],
              shaded_g[idx],
              shaded_b[idx]);
  }
}
// void render_cube_frame(float angleX, float angleY)
// {
//     vertex_proj_t proj[8];

//     // 회전행렬 요소 (Y -> X 순으로 회전)
//     float cx = cosf(angleX);
//     float sx = sinf(angleX);
//     float cy = cosf(angleY);
//     float sy = sinf(angleY);

//     // 간단한 카메라/투영 세팅
//     float camera_z = 3.5f;     // 카메라와 큐브 중심 거리 (앞으로 이동)
//     float scale    = 33.0f;    // 화면 확대 배율

//     for (int i = 0; i < 8; i++) {
//         vec3f_t v = cube_vertices[i];

//         // Y축 회전
//         float x1 =  v.x * cy + v.z * sy;
//         float z1 = -v.x * sy + v.z * cy;
//         float y1 =  v.y;

//         // X축 회전
//         float y2 =  y1 * cx - z1 * sx;
//         float z2 =  y1 * sx + z1 * cx;
//         float x2 =  x1;

//         // 카메라 기준으로 앞으로 이동 (z가 0 근처가 되지 않게 offset)
//         float z_cam = z2 + camera_z;

//         // 퍼스펙티브 투영
//         float invz = 1.0f / z_cam;
//         float xp = x2 * invz * scale;
//         float yp = y2 * invz * scale;

//         int16_t sx_i = (int16_t)(SCREEN_W  / 2 + xp);
//         int16_t sy_i = (int16_t)(SCREEN_H / 2 - yp);

//         proj[i].x = sx_i;
//         proj[i].y = sy_i;
//         proj[i].z = z_cam; // painter용 깊이
//     }

//     // 각 면의 평균 깊이 계산 후 정렬
//     face_order_t order[6];
//     for (int i = 0; i < 6; i++) {
//         const face_t *f = &cube_faces[i];
//         float zsum = 0.0f;
//         for (int k = 0; k < 4; k++) {
//             zsum += proj[f->idx[k]].z;
//         }
//         order[i].face_index = (uint8_t)i;
//         order[i].depth      = zsum * 0.25f;
//     }

// 		    // 회전된 노멀 (카메라 공간)
//     vec3f_t face_normals_cam[6];

//     for (int i = 0; i < 6; i++) {
//         vec3f_t n = face_normals_model[i];

//         // Y축 회전
//         float x1 =  n.x * cy + n.z * sy;
//         float z1 = -n.x * sy + n.z * cy;
//         float y1 =  n.y;

//         // X축 회전
//         float y2 =  y1 * cx - z1 * sx;
//         float z2 =  y1 * sx + z1 * cx;
//         float x2 =  x1;

//         face_normals_cam[i].x = x2;
//         face_normals_cam[i].y = y2;
//         face_normals_cam[i].z = z2;
//     }

//     // 조명 방향 (살짝 위, 오른쪽, 앞에서 비추는 느낌)
//     vec3f_t light_dir = { 0.4f, 0.8f, -0.6f };

//     // 정규화
//     float len = sqrtf(light_dir.x * light_dir.x +
//                       light_dir.y * light_dir.y +
//                       light_dir.z * light_dir.z);
//     light_dir.x /= len;
//     light_dir.y /= len;
//     light_dir.z /= len;

//     // 라이트 색 (살짝 따뜻한 톤)
//     float light_r = 1.0f;
//     float light_g = 0.95f;
//     float light_b = 0.90f;

//     uint8_t shaded_r[6], shaded_g[6], shaded_b[6];

//     for (int i = 0; i < 6; i++) {
//         vec3f_t n = face_normals_cam[i];

//         // 노멀 정규화
//         float nlen = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
//         if (nlen > 0.0f) {
//             n.x /= nlen;
//             n.y /= nlen;
//             n.z /= nlen;
//         }

//         // diffuse: n · L
//         float ndotl = n.x * light_dir.x +
//                       n.y * light_dir.y +
//                       n.z * light_dir.z;
//         if (ndotl < 0.0f) ndotl = 0.0f;

//         // ambient + diffuse
//         float ambient = 0.25f;        // 기본 밝기
//         float diffuse = ndotl;        // 0 ~ 1

//         // 간단한 specular (하이라이트)
//         vec3f_t view_dir = { 0.0f, 0.0f, -1.0f }; // z-축 뒤에서 보는 카메라라고 가정

//         // r = 2(n·l)n - l
//         vec3f_t r;
//         float two_nl = 2.0f * ndotl;
//         r.x = two_nl * n.x - light_dir.x;
//         r.y = two_nl * n.y - light_dir.y;
//         r.z = two_nl * n.z - light_dir.z;

//         // 정규화 후 r·v
//         float rlen = sqrtf(r.x * r.x + r.y * r.y + r.z * r.z);
//         if (rlen > 0.0f) {
//             r.x /= rlen;
//             r.y /= rlen;
//             r.z /= rlen;
//         }

//         float rv = r.x * view_dir.x +
//                    r.y * view_dir.y +
//                    r.z * view_dir.z;
//         if (rv < 0.0f) rv = 0.0f;

//         float specular = powf(rv, 16.0f);  // 지수 높을수록 반짝이는 영역이 작아짐

//         // 전체 밝기 계수
//         float k = ambient + 0.7f * diffuse + 0.4f * specular;
//         if (k > 1.0f) k = 1.0f;

//         const face_t *f = &cube_faces[i];

//         // base color -> [0,1] 로 바꾸고, 라이트 색 + 밝기 반영
//         float cr = (float)f->r / 255.0f;
//         float cg = (float)f->g / 255.0f;
//         float cb = (float)f->b / 255.0f;

//         float out_r = cr * light_r * k;
//         float out_g = cg * light_g * k;
//         float out_b = cb * light_b * k;

//         if (out_r > 1.0f) out_r = 1.0f;
//         if (out_g > 1.0f) out_g = 1.0f;
//         if (out_b > 1.0f) out_b = 1.0f;

//         shaded_r[i] = (uint8_t)(out_r * 255.0f);
//         shaded_g[i] = (uint8_t)(out_g * 255.0f);
//         shaded_b[i] = (uint8_t)(out_b * 255.0f);
//     }

//     sort_faces_by_depth(order, 6);

//     // 먼 면부터 차례대로 채우기
//     for (int i = 0; i < 6; i++) {
//         int idx = order[i].face_index;
//         const face_t *f = &cube_faces[idx];
//         fill_face(proj, f,
//                   shaded_r[idx],
//                   shaded_g[idx],
//                   shaded_b[idx]);
//     }
// }