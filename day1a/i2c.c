#define F_CPU 8000000L

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#define I2C_PORT PORTB
#define I2C_DDR DDRB
#define I2C_PIN PINB
#define SDA_PIN PB0 // 5
#define SCL_PIN PB2 // 7

// --- I2C (유사) 비트 뱅잉 함수 ---

void I2C_Init()
{
  // SCL, SDA를 출력으로 설정하고 High 상태 유지
  I2C_DDR |= (1 << SCL_PIN) | (1 << SDA_PIN);
  I2C_PORT |= (1 << SCL_PIN) | (1 << SDA_PIN);
}

void I2C_Start()
{
  // SDA High, SCL High 상태에서 SDA를 Low로
  I2C_PORT |= (1 << SDA_PIN);
  I2C_PORT |= (1 << SCL_PIN);
  _delay_us(4);
  I2C_PORT &= ~(1 << SDA_PIN);
  _delay_us(4);
  I2C_PORT &= ~(1 << SCL_PIN); // 준비 상태
}

void I2C_Stop()
{
  // SCL High 상태에서 SDA를 Low에서 High로
  I2C_PORT &= ~(1 << SDA_PIN);
  I2C_PORT |= (1 << SCL_PIN);
  _delay_us(4);
  I2C_PORT |= (1 << SDA_PIN);
  _delay_us(4);
}

uint8_t I2C_WriteByte(uint8_t data)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // SCL Low 상태에서 데이터 변경
    I2C_PORT &= ~(1 << SCL_PIN);

    // MSB부터 전송 (TM1650 데이터시트 p.4 참조)
    if (data & 0x80)
    {
      I2C_PORT |= (1 << SDA_PIN);
    }
    else
    {
      I2C_PORT &= ~(1 << SDA_PIN);
    }
    _delay_us(2);

    // SCL High (데이터 래치)
    I2C_PORT |= (1 << SCL_PIN);
    _delay_us(2);
    I2C_PORT &= ~(1 << SCL_PIN);

    data <<= 1;
  }

  // ACK 읽기
  I2C_DDR &= ~(1 << SDA_PIN);
  _delay_us(2);
  I2C_PORT |= (1 << SCL_PIN);
  _delay_us(2);
  uint8_t ack = (I2C_PIN & (1 << SDA_PIN)) ? 0 : 1; // ACK는 Low
  I2C_PORT &= ~(1 << SCL_PIN);
  I2C_DDR |= (1 << SDA_PIN);

  return ack; // 1=성공, 0=실패
}

uint8_t I2C_ReadByte(uint8_t ack)
{
  uint8_t data = 0;

  // SDA 입력으로 설정
  I2C_DDR &= ~(1 << SDA_PIN);

  for (uint8_t i = 0; i < 8; i++)
  {

    // SCL Low (안정화)
    I2C_PORT &= ~(1 << SCL_PIN);
    _delay_us(2);

    // SCL High → SDA 읽기
    I2C_PORT |= (1 << SCL_PIN);
    _delay_us(2);

    data <<= 1;
    if (I2C_PIN & (1 << SDA_PIN))
    {
      data |= 1;
    }

    // SCL Low
    I2C_PORT &= ~(1 << SCL_PIN);
  }

  // --- ACK/NACK 출력 단계 ---
  I2C_DDR |= (1 << SDA_PIN); // SDA를 출력으로 전환

  if (ack)
  {
    I2C_PORT &= ~(1 << SDA_PIN); // ACK = 0
  }
  else
  {
    I2C_PORT |= (1 << SDA_PIN); // NACK = 1
  }
  _delay_us(2);

  // SCL High → ACK/NACK 전송
  I2C_PORT |= (1 << SCL_PIN);
  _delay_us(2);

  I2C_PORT &= ~(1 << SCL_PIN);

  // SDA 다시 High 로 풀기
  I2C_PORT |= (1 << SDA_PIN);

  return data;
}