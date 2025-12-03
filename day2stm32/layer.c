#include "layer.h"

layer_t layers[MAX_LAYER];
static uint8_t layer_amount = 0;

void layer_clear(void) { layer_amount = 0; }

uint64_t layer_move(void) // 레이어 이동 반영 후 업데이트된 row를 64비트 형태로 반환
{
  uint64_t update = 0;

  for (int i = 0; i < layer_amount; i++)
  {
    // --- X 이동 + 반사 ---
    layers[i].x += layers[i].dx;

    if (layers[i].x > POSMAX)
    {
      layers[i].x = POSMAX;
      layers[i].dx = -layers[i].dx; // 경계에서 튕김
    }
    else if (layers[i].x < POSMIN)
    {
      layers[i].x = POSMIN;
      layers[i].dx = -layers[i].dx;
    }

    // --- Y 이동 + 반사 ---
    layers[i].y += layers[i].dy;

    if (layers[i].y > POSMAX)
    {
      layers[i].y = POSMAX;
      layers[i].dy = -layers[i].dy;
    }
    else if (layers[i].y < POSMIN)
    {
      layers[i].y = POSMIN;
      layers[i].dy = -layers[i].dy;
    }

    // 이 레이어 때문에 영향 받는 y 구간 (±5) 계산
    int y0 = layers[i].y - 5;
    int y1 = layers[i].y + 5;

    if (y0 < 0) y0 = 0;
    if (y1 > 63) y1 = 63;

    for (int b = y0; b <= y1; b++)
    {
      update |= (1ULL << b); // 64비트 마스크
    }
  }

  return update;
}

row_t layer_capture_row(int r)
{
  row_t row;
  // 1. 초기화
  for (int x = 0; x < 64; ++x)
    row.r[x] = row.g[x] = row.b[x] = 0;

  // 2. 레이어를 순서대로 합성
  //    layer_add() 가 뒤에 추가하니까
  //    0 -> 1 -> ... -> layer_amount-1 순으로 돌면
  //    "나중에 추가된 레이어"가 자연스럽게 위에 덮임
  for (int li = 0; li < layer_amount; ++li)
  {
    layer_t *L = &layers[li];

    // 이 레이어가 커버하는 y 범위: [y-3, y+3] (총 7줄)
    int top = L->y - 3;
    int rel_y = r - top; // 0 ~ 6

    if (rel_y < 0 || rel_y >= 7)
      continue; // 이 레이어는 이 줄에 없음

    uint8_t mr = L->r[rel_y]; // 이 줄의 R 마스크 (8bit)
    uint8_t mg = L->g[rel_y]; // G 마스크
    uint8_t mb = L->b[rel_y]; // B 마스크

    // 가로 8칸 사용 (왼쪽 끝 안 잘리게)
    // 중심을 L->x 근처로 두고 싶으면:
    //   [x-3 .. x+4] 또는 [x-4 .. x+3] 중 택1
    // 여기선 x-3 .. x+4 로 사용
    int left = L->x - 3; // sx=0일 때 화면 x 좌표

    for (int sx = 0; sx < 8; ++sx)
    {
      int x = left + sx;
      if (x < 0 || x >= 64) continue;

      // randsh에서 MSB(비트7)가 가장 왼쪽이라 가정
      uint8_t mask = 1u << (7 - sx);

      // 이 레이어가 이 픽셀에 뭔가 그리면 "덮어쓴다"
      // (겹쳐서 밝아지지 않고, 나중 레이어가 위로 올라오게)
      if (mr & mask) row.r[x] = 255;
      if (mg & mask) row.g[x] = 255;
      if (mb & mask) row.b[x] = 255;
    }
  }

  return row;
}

void layer_add(layer_t l)
{
  layers[layer_amount++] = l;
}

// 랜덤 방향(사실 수도랜덤)
static const int8_t randdx[] = {1, 1, -1, -1, 2, -2, 0, 0, 2, 2, -2, -2, 1, -1, 1, -1};
static const int8_t randdy[] = {1, -1, 1, -1, 0, 0, -2, 2, 1, -1, 1, -1, 2, 2, -2, -2};
static const uint8_t randdirmax = 16;
static uint8_t randdirnow = 0;

// 랜덤 모양
static const uint8_t randshmax = 9;
static uint8_t randshnow = 0;
static const uint8_t randsh[9][7] = {
    // ⭐ 별
    {
        0b00011000,
        0b00111100,
        0b11111111,
        0b01111110,
        0b00111100,
        0b01100110,
        0b11000011},

    // ❤️ 하트
    {
        0b01100110,
        0b11111111,
        0b11111111,
        0b11111111,
        0b01111110,
        0b00111100,
        0b00011000},

    // ◼ 네모 (정사각형)
    {
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111},

    // ▲ 세모 (위 방향)
    {
        0b00011000,
        0b00011000,
        0b00111100,
        0b00111100,
        0b01111110,
        0b01111110,
        0b11111111},

    // 세모(아래 방향)
    {
        0b11111111,
        0b01111110,
        0b01111110,
        0b00111100,
        0b00111100,
        0b00011000,
        0b00011000},

    // ✚ 플러스
    {
        0b00011000,
        0b00011000,
        0b00011000,
        0b11111111,
        0b00011000,
        0b00011000,
        0b00011000},

    // 다이아몬드
    {
        0b00011000,
        0b00111100,
        0b01111110,
        0b11111111,
        0b01111110,
        0b00111100,
        0b00011000},
    // ❤️ 하트
    {
        0b01100110,
        0b11111111,
        0b11111111,
        0b11111111,
        0b01111110,
        0b00111100,
        0b00011000},

    // ✖ X자
    {
        0b11000011,
        0b11100111,
        0b01111110,
        0b00111100,
        0b00111100,
        0b11100111,
        0b11000011}};

// 랜덤 컬러
static const uint8_t randcolr[7] = {255, 0, 0, 255, 255, 0, 255};
static const uint8_t randcolg[7] = {0, 255, 0, 255, 0, 255, 255};
static const uint8_t randcolb[7] = {0, 0, 255, 0, 255, 255, 255};
static const uint8_t randcolmax = 7;
static uint8_t randcolnow = 0;

// 랜덤 좌표
static const uint8_t randposx[4] = {10, 20, 30, 40};
static const uint8_t randposy[4] = {20, 50, 30, 45};
static const uint8_t randposmax = 4;
static uint8_t randposnow = 0;

void layer_add_random(void)
{
  if (layer_amount == MAX_LAYER) return;
  layer_t l = {
      randposx[randposnow],
      randposy[randposnow],
      randdx[randdirnow],
      randdy[randdirnow],
  };
  for (int r = 0; r < 7; r++)
  {
    l.r[r] = randcolr[randcolnow] & randsh[randshnow][r];
    l.g[r] = randcolg[randcolnow] & randsh[randshnow][r];
    l.b[r] = randcolb[randcolnow] & randsh[randshnow][r];
  }
  if (++randdirnow == randdirmax) randdirnow = 0;
  if (++randshnow == randshmax) randshnow = 0;
  if (++randcolnow == randcolmax) randcolnow = 0;
  if (++randposnow == randposmax) randposnow = 0;
  layer_add(l);
}