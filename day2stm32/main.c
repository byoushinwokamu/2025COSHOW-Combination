/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body (HUB75 LED Matrix Control)
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// --- PIN DEFINITIONS ---
// RGB Data: PA0~PA5
#define RGB_MASK 0x003F
// Address: PA8~PA11 (A, B, C, D)
#define ADDR_MASK 0x0F00
#define ADDR_SHIFT 8

// Control Pins (Port B)
#define PIN_OE GPIO_PIN_5
#define PIN_CLK GPIO_PIN_7
#define PIN_LAT GPIO_PIN_8

// --- SETTINGS ---
#define PANEL_WIDTH_TOTAL 128 // 64픽셀 패널 2개 연결
#define BAM_DELAY_BASE 40     // 84MHz 기준 밝기 딜레이

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
// 애니메이션 시작 색상 인덱스 (가로 쉬프트 효과를 위한 전역 변수)
uint8_t row_buffer[384]; // 128 pixel * 3 planes
uint32_t timer_tick = 0;
uint8_t test_mode = 0;
row_t scan_rows[4];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
void delay_cycles(uint32_t count);
void render_row(uint8_t row);
void update_buffer_pattern(uint8_t row);
void update_buffer_from_frame(uint8_t row);
void update_buffer_from_layers(uint8_t rowAddr);
void process_layer_update(uint64_t layer_update);
void hub75_update_from_layers(uint64_t layer_update);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define PANEL_WIDTH 64 // 가로 64컬럼
#define PANEL_WIDTH_TOTAL 128

// ---- 핀 제어 헬퍼 ----
static inline void set_color_top_raw(uint8_t r, uint8_t g, uint8_t b)
{
  HAL_GPIO_WritePin(R1_GPIO_Port, R1_Pin, r ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(G1_GPIO_Port, G1_Pin, g ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(B1_GPIO_Port, B1_Pin, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static inline void set_color_bottom_raw(uint8_t r, uint8_t g, uint8_t b)
{
  HAL_GPIO_WritePin(R2_GPIO_Port, R2_Pin, r ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(G2_GPIO_Port, G2_Pin, g ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(B2_GPIO_Port, B2_Pin, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static inline void clk_pulse(void)
{
  HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_SET);
  __NOP();
  __NOP();
  HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_RESET);
}

static inline void latch_pulse(void)
{
  HAL_GPIO_WritePin(LAT_GPIO_Port, LAT_Pin, GPIO_PIN_SET);
  __NOP();
  __NOP();
  HAL_GPIO_WritePin(LAT_GPIO_Port, LAT_Pin, GPIO_PIN_RESET);
}

// OE는 보통 Active Low (Low일 때 LED ON)
static inline void oe_on(void)
{
  HAL_GPIO_WritePin(OE_GPIO_Port, OE_Pin, GPIO_PIN_RESET);
}

static inline void oe_off(void)
{
  HAL_GPIO_WritePin(OE_GPIO_Port, OE_Pin, GPIO_PIN_SET);
}

// 행 주소 (일단 1/16 스캔 가정, 0~15 사용)
static inline void set_row_addr(uint8_t row)
{
  row &= 0x0F; // 0~15

  A_GPIO_Port->ODR = (A_GPIO_Port->ODR & 0xfffff0ff) | (row << 8);
  // HAL_GPIO_WritePin(A_GPIO_Port, A_Pin, (row & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  // HAL_GPIO_WritePin(B_GPIO_Port, B_Pin, (row & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  // HAL_GPIO_WritePin(C_GPIO_Port, C_Pin, (row & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  // HAL_GPIO_WritePin(D_GPIO_Port, D_Pin, (row & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  // 2. TIM1 Channel 3 (PA10, HUB75 OE 핀) PWM 출력 시작
  // HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  oe_off();
  HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LAT_GPIO_Port, LAT_Pin, GPIO_PIN_RESET);
  float ax = 0, ay = 0;
  uint8_t prevsw = -1;
  uint16_t update_count = 0;
  uint8_t mode = 0; // 0: cube, 1: layer
  uint8_t cubestop = 0;
  uint64_t update_flag = 0;
  while (1)
  {
    // cube
    if (mode == 0)
    {
      clear_framebuffer();
      render_cube_frame(ax, ay);
      for (uint8_t row = 0; row < 16; row++)
      {
        update_buffer_from_frame(row);
        render_row(row); // 패널 전송
      }
      oe_off();
      if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET)
      {
        ax += 0.015f;
        ay += 0.021f;
        prevsw = 0;
      }
      else if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET)
      {
        if (prevsw != 1)
        {
          prevsw = 1;
          mode = 1;
        }
      }
      else prevsw = -1;
    }

    // layer
    if (mode == 1)
    {
      if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET)
      {
        if (prevsw != 0)
        {
          prevsw = 0;
          layer_add_random();
        }
      }
      else if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET)
      {
        if (prevsw != 1)
        {
          prevsw = 1;
          layer_clear();
          mode = 0;
        }
      }
      else prevsw = -1;
      if (update_count++ == 16000)
      {
        update_count = 0;
        update_flag = layer_move();
        hub75_update_from_layers(update_flag);
      }
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief TIM3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 16;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 100;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, R1_Pin | G1_Pin | B1_Pin | R2_Pin | G2_Pin | B2_Pin | A_Pin | B_Pin | C_Pin | D_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12 | OE_Pin | CLK_Pin | LAT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : R1_Pin G1_Pin B1_Pin R2_Pin
                           G2_Pin B2_Pin A_Pin B_Pin
                           C_Pin D_Pin */
  GPIO_InitStruct.Pin = R1_Pin | G1_Pin | B1_Pin | R2_Pin | G2_Pin | B2_Pin | A_Pin | B_Pin | C_Pin | D_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SW0_Pin SW1_Pin */
  GPIO_InitStruct.Pin = SW0_Pin | SW1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 OE_Pin CLK_Pin LAT_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_12 | OE_Pin | CLK_Pin | LAT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// --- Helper Functions ---
void delay_cycles(uint32_t count)
{
  while (count--)
  {
    __asm("nop");
  }
}

// --- Render Logic (Direct Register Access) ---
void render_row(uint8_t row)
{
  // 1. OE High (LED Off)
  GPIOB->BSRR = PIN_OE;

  // 2. Set Row Address (PA8~11)
  GPIOA->ODR = (GPIOA->ODR & ~ADDR_MASK) | ((row & 0x0F) << ADDR_SHIFT);

  // 3. BAM (Bit Angle Modulation)
  for (uint8_t plane = 0; plane < 3; plane++)
  {
    uint8_t *ptr = &row_buffer[plane * PANEL_WIDTH_TOTAL];

    for (uint8_t col = 0; col < PANEL_WIDTH_TOTAL; col++)
    {
      GPIOB->BSRR = (PIN_CLK << 16); // CLK Low

      // Set RGB Data (PA0~5)
      GPIOA->ODR = (GPIOA->ODR & ~RGB_MASK) | (*ptr++ & RGB_MASK);

      GPIOB->BSRR = PIN_CLK; // CLK High
    }

    // Latch Data
    GPIOB->BSRR = PIN_LAT;
    GPIOB->BSRR = (PIN_LAT << 16);

    // OE Low (Display On)
    GPIOB->BSRR = (PIN_OE << 16);

    delay_cycles(BAM_DELAY_BASE * (1 << plane));

    // OE High (Display Off)
    GPIOB->BSRR = PIN_OE;
  }
}

// --- Test Pattern Logic ---
void update_buffer_pattern(uint8_t row)
{
  memset(row_buffer, 0, sizeof(row_buffer));

  uint8_t *p0 = &row_buffer[0];
  uint8_t *p1 = &row_buffer[PANEL_WIDTH_TOTAL];
  uint8_t *p2 = &row_buffer[PANEL_WIDTH_TOTAL * 2];

  for (int x = 0; x < PANEL_WIDTH_TOTAL; x++)
  {
    uint8_t r = 0, g = 0, b = 0;

    switch (test_mode)
    {
    case 0:
      r = 7;
      break; // RED
    case 1:
      g = 7;
      break; // GREEN
    case 2:
      b = 7;
      break; // BLUE
    case 3:
      r = 7;
      g = 7;
      b = 7;
      break; // WHITE
    case 4:  // Gradient
      if (x < 42)
      {
        r = x % 8;
      }
      else if (x < 84)
      {
        g = x % 8;
      }
      else
      {
        b = x % 8;
      }
      break;
    }

    uint8_t val0 = 0, val1 = 0, val2 = 0;
    // 상단(Row), 하단(Row+16) 동시 제어 매핑
    if (r & 1) val0 |= 0x09;
    if (g & 1) val0 |= 0x12;
    if (b & 1) val0 |= 0x24;
    if (r & 2) val1 |= 0x09;
    if (g & 2) val1 |= 0x12;
    if (b & 2) val1 |= 0x24;
    if (r & 4) val2 |= 0x09;
    if (g & 4) val2 |= 0x12;
    if (b & 4) val2 |= 0x24;

    *p0++ = val0;
    *p1++ = val1;
    *p2++ = val2;
  }
}

#define PANEL_WIDTH 64        // 패널 한 개 가로
#define PANEL_WIDTH_TOTAL 128 // 두 개 캐스케이드
#define PANEL_HEIGHT 64       // 논리 해상도
#define SCAN_LINES 16         // 1/16 스캔
// row: 스캔라인 번호 (0 ~ SCAN_LINES-1)
// 실제로는 위쪽 row, 아래쪽 row+SCAN_LINES 두 줄을 동시에 다룬다고 가정
void update_buffer_from_frame(uint8_t row)
{
  memset(row_buffer, 0, sizeof(row_buffer));

  uint8_t *p0 = &row_buffer[0];                     // plane 0
  uint8_t *p1 = &row_buffer[PANEL_WIDTH_TOTAL];     // plane 1
  uint8_t *p2 = &row_buffer[PANEL_WIDTH_TOTAL * 2]; // plane 2

  for (int col = 0; col < PANEL_WIDTH_TOTAL; col++)
  {

    int x;
    uint8_t phys_y_top; // 패널에 실제로 점등되는 y
    uint8_t phys_y_bot;

    if (col < PANEL_WIDTH)
    {
      // 왼쪽 패널
      x = col - PANEL_WIDTH;
      phys_y_top = row + 32; // 32~47
      phys_y_bot = row + 48; // 48~63
    }
    else
    {
      // 오른쪽 패널
      x = col;
      phys_y_top = row;      // 0~15
      phys_y_bot = row + 16; // 16~31
    }

    // 여기서 "논리 좌표"로 뒤집기 (위/아래 반전)
    uint8_t y_top = (PANEL_HEIGHT - 1) - phys_y_top; // 63 - phys_y_top
    uint8_t y_bot = (PANEL_HEIGHT - 1) - phys_y_bot; // 63 - phys_y_bot

    // 이제 fb는 y=0이 "논리적으로 위쪽"이라고 가정
    uint8_t r_top = fb[y_top].r[x];
    uint8_t g_top = fb[y_top].g[x];
    uint8_t b_top = fb[y_top].b[x];

    uint8_t r_bot = fb[y_bot].g[x];
    uint8_t g_bot = fb[y_bot].b[x];
    uint8_t b_bot = fb[y_bot].r[x];

    uint8_t val0 = 0, val1 = 0, val2 = 0;

    // --- 비트 매핑 (기존 테스트 패턴과 동일한 구조) ---
    // plane 0
    if (r_top & 1) val0 |= 0x01;
    if (g_top & 1) val0 |= 0x02;
    if (b_top & 1) val0 |= 0x04;

    if (r_bot & 1) val0 |= 0x08;
    if (g_bot & 1) val0 |= 0x10;
    if (b_bot & 1) val0 |= 0x20;

    // plane 1
    if (r_top & 2) val1 |= 0x01;
    if (g_top & 2) val1 |= 0x02;
    if (b_top & 2) val1 |= 0x04;

    if (r_bot & 2) val1 |= 0x08;
    if (g_bot & 2) val1 |= 0x10;
    if (b_bot & 2) val1 |= 0x20;

    // plane 2
    if (r_top & 4) val2 |= 0x01;
    if (g_top & 4) val2 |= 0x02;
    if (b_top & 4) val2 |= 0x04;

    if (r_bot & 4) val2 |= 0x08;
    if (g_bot & 4) val2 |= 0x10;
    if (b_bot & 4) val2 |= 0x20;

    *p0++ = val0;
    *p1++ = val1;
    *p2++ = val2;
  }
}
// 레이어로부터 해당 scan address(rowAddr)에 필요한 4개 논리 row를 캡처해서
// HUB75용 row_buffer에 패킹
void update_buffer_from_layers(uint8_t rowAddr)
{
  // 1. 이 addr에 해당하는 논리 y들
  uint8_t y2 = rowAddr;      // 위쪽 패널 상단
  uint8_t y3 = rowAddr + 16; // 위쪽 패널 하단
  uint8_t y0 = rowAddr + 32; // 아래쪽 패널 상단
  uint8_t y1 = rowAddr + 48; // 아래쪽 패널 하단

  // 2. 각 row를 layer 시스템에서 캡처
  row_t row0 = layer_capture_row(y0);
  row_t row1 = layer_capture_row(y1);
  row_t row2 = layer_capture_row(y2);
  row_t row3 = layer_capture_row(y3);

  // 3. HUB75 row_buffer 초기화
  memset(row_buffer, 0, sizeof(row_buffer));

  uint8_t *p0 = &row_buffer[0];                     // plane 0 (LSB)
  uint8_t *p1 = &row_buffer[PANEL_WIDTH_TOTAL];     // plane 1
  uint8_t *p2 = &row_buffer[PANEL_WIDTH_TOTAL * 2]; // plane 2

  for (int col = 0; col < PANEL_WIDTH_TOTAL; col++)
  {

    int x;
    // top/bottom 픽셀의 3bit RGB
    uint8_t r_top, g_top, b_top;
    uint8_t r_bot, g_bot, b_bot;

    if (col < PANEL_WIDTH)
    {
      // 왼쪽 64픽셀 = 위 패널 (논리 y0,y1)
      x = col;

      r_top = (row0.r[x] ? 7 : 0);
      g_top = (row0.g[x] ? 7 : 0);
      b_top = (row0.b[x] ? 7 : 0);

      r_bot = (row1.r[x] ? 7 : 0);
      g_bot = (row1.g[x] ? 7 : 0);
      b_bot = (row1.b[x] ? 7 : 0);
    }
    else
    {
      // 오른쪽 64픽셀 = 아래 패널 (논리 y2,y3)
      x = col - PANEL_WIDTH;

      r_top = (row2.r[x] ? 7 : 0);
      g_top = (row2.g[x] ? 7 : 0);
      b_top = (row2.b[x] ? 7 : 0);

      r_bot = (row3.r[x] ? 7 : 0);
      g_bot = (row3.g[x] ? 7 : 0);
      b_bot = (row3.b[x] ? 7 : 0);
    }

    uint8_t val0 = 0, val1 = 0, val2 = 0;

    // plane 0 (LSB)
    if (r_top & 1) val0 |= 0x01;
    if (g_top & 1) val0 |= 0x02;
    if (b_top & 1) val0 |= 0x04;

    if (r_bot & 1) val0 |= 0x08;
    if (g_bot & 1) val0 |= 0x10;
    if (b_bot & 1) val0 |= 0x20;

    // plane 1
    if (r_top & 2) val1 |= 0x01;
    if (g_top & 2) val1 |= 0x02;
    if (b_top & 2) val1 |= 0x04;

    if (r_bot & 2) val1 |= 0x08;
    if (g_bot & 2) val1 |= 0x10;
    if (b_bot & 2) val1 |= 0x20;

    // plane 2
    if (r_top & 4) val2 |= 0x01;
    if (g_top & 4) val2 |= 0x02;
    if (b_top & 4) val2 |= 0x04;

    if (r_bot & 4) val2 |= 0x08;
    if (g_bot & 4) val2 |= 0x10;
    if (b_bot & 4) val2 |= 0x20;

    *p0++ = val0;
    *p1++ = val1;
    *p2++ = val2;
  }
}

void hub75_update_from_layers(uint64_t layer_update)
{
  // y가 속한 addr = y % 16
  uint16_t addr_dirty = 0;

  // 1. 바뀐 row(y)들을 보고, 해당 addr들을 표시
  for (int y = 0; y < 64; ++y)
  {
    if (layer_update & (1ULL << y))
    {
      int addr = y & 0x0F; // y % 16
      addr_dirty |= (1u << addr);
    }
  }

  // 2. 실제로 바뀐 addr 그룹만 HUB75로 전송
  for (int addr = 0; addr < 16; ++addr)
  {
    if (!(addr_dirty & (1u << addr)))
      continue; // 이 주소 그룹은 이번 프레임에서 변화 없음

    // 이 addr에 대해서:
    //  - layer_capture_row(y0..y3)로 4개 논리 row 캡처해서 row_buffer 채우고
    //  - render_row(addr)로 HUB75에 쏴준다
    update_buffer_from_layers((uint8_t)addr);
    render_row((uint8_t)addr);
  }
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
