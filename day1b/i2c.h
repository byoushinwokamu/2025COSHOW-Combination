#ifndef _I2C_H_
#define _I2C_H_

#include <stdint.h>

void I2C_Init();
void I2C_Start();
void I2C_Stop();
uint8_t I2C_WriteByte(uint8_t data);
uint8_t I2C_ReadByte(uint8_t ack);

#endif