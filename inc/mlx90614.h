#ifndef __MLX90614_H__
#define __MLX90614_H__

#include "stm32f10x.h"

#define SMBUS_SCL_1 GPIOB->BSRR = (1 << 6)
#define SMBUS_SCL_0 GPIOB->BRR = (1 << 6)
#define SMBUS_SDA_1 GPIOB->BSRR = (1 << 7)
#define SMBUS_SDA_0 GPIOB->BRR = (1 << 7)
#define SMBUS_READ_SDA() ((GPIOB->IDR) & (1 << 7))

#define US 2

typedef enum Bit
{
	ACK = 0 ,
	NACK
}bit_t;



void SMBus_Init(void);
void SMBus_Start(void);
uint8_t SMBus_SendByte(uint8_t byte);
uint8_t WaitACK(void);
void SMBus_SendBit(bit_t bit);
uint8_t SMBus_ReadByte(void);
void SMBus_Stop(void);
unsigned char CRC8(unsigned char *ptr,unsigned char len);

uint16_t GetSlaveAddress(void);
uint8_t SetSlaveAddress(uint8_t slave_address);
uint16_t GetTemp(uint8_t slave_address);
void SetEmissivity(uint8_t slave_address, float emissivity);
uint16_t GetEmissivity(uint8_t slave_address);

#endif
