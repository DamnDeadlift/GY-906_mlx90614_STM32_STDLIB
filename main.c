/*
** MLX90614测量温度，并通过串口打印出来
*/

#include "stm32f10x.h"
#include "my_uart.h"

#define ID_ADDRESS 0x1C

#define US 2

#define SMBUS_SCL_1 GPIOB->BSRR = (1 << 6)
#define SMBUS_SCL_0 GPIOB->BRR = (1 << 6)
#define SMBUS_SDA_1 GPIOB->BSRR = (1 << 7)
#define SMBUS_SDA_0 GPIOB->BRR = (1 << 7)
#define SMBUS_READ_SDA() ((GPIOB->IDR) & (1 << 7))

typedef enum Bit
{
	ACK = 0 ,
	NACK
}bit_t;

uint8_t crc_buffer [5] = {0x00,0x24,0x00,0x00,0x00};
uint8_t cnt = 0;

void GPIO_Config(void);
void Basic_Timer_Config(void);
void Delay_ms(uint16_t ms);
void Delay_us(uint16_t us);

void HexToBin(uint8_t byte, uint8_t bin[]);

void SMBus_Init(void);
void SMBus_Start(void);

//返回ACK的值，0为ack，1为nack
uint8_t SMBus_SendByte(uint8_t byte);

void SMBus_SendBit(bit_t bit);
uint8_t SMBus_ReadByte(void);
void SMBus_Stop(void);
uint8_t WaitACK(void);

int16_t GetSlaveAddress(void);
uint16_t GetTemp(void);
void SetEmissivity(float emissivity);
uint16_t GetEmissivity(void);


unsigned char CRC8(unsigned char *ptr,unsigned char len)
{
 
    unsigned char crc;
    unsigned char i;
    crc = 0;
    while(len--)
    {
       crc ^= *ptr++;
       for(i = 0;i < 8;i++)
       {
           if(crc & 0x80)
           {
               crc = (crc << 1) ^ 0x07;
           }
           else 
		   {
			   crc <<= 1;
		   }
       }
    }
    return crc;
 
}

volatile uint16_t count = 0;

int main(void)
{
	uint16_t slave_address = 0;
	uint16_t temp = 0;
	uint16_t emissivity = 0;
	float temperture = 0.0;
	GPIO_Config();
	UART_Config(115200);
	Basic_Timer_Config();
	
	UART_PRINTF("START!!!\n");
	
	slave_address = GetSlaveAddress();
	UART_PRINTF("SLAVE ADDRESS = %#x\n", slave_address);
	
	SetEmissivity(0);
	Delay_us(6000);
	
	SetEmissivity(0.99);
	Delay_us(6000);
	
	while(1)
	{
		temp = GetTemp();
		temperture = temp * 0.02 - 273.15;
		UART_PRINTF("temp = %.2f\n", temperture);	
		cnt = 0;
		//break;
	}
		
}



void GPIO_Config(void)
{
	
	GPIO_InitTypeDef GPIO_InitStruct;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Basic_Timer_Config(void)
{
	TIM_TimeBaseInitTypeDef Timer_InitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;
	
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	
	Timer_InitStruct.TIM_Prescaler = 71;
	Timer_InitStruct.TIM_Period = 1;
	TIM_TimeBaseInit(TIM6, &Timer_InitStruct);
	
	TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM6, ENABLE);
	
	NVIC_InitStruct.NVIC_IRQChannel = TIM6_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	//NVIC_SetPriorityGrouping(NVIC_PriorityGroup_1);
	NVIC_Init(&NVIC_InitStruct);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, DISABLE);
	
}

void Delay_ms(uint16_t ms)
{
	//TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	while(count != ms);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, DISABLE);
	count = 0;
}

void Delay_us(uint16_t us)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	while(count != us);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, DISABLE);
	count = 0;
}

//void HexToBin(uint8_t byte, uint8_t bin[])
//{
//	uint8_t i = 0;
//	for(i = 0; i < 8; i++)
//	{
//		bin[i] = (byte & (1 << i)) >> i;
//	}
//}

void SMBus_Init(void)
{
	SMBUS_SCL_1;
	SMBUS_SDA_1;
	Delay_us(US);
	
	SMBUS_SCL_0;
	Delay_us(1400);
	
	SMBUS_SCL_1;
	Delay_us(US);
}

void SMBus_Start(void)
{
	
	SMBUS_SDA_1;
	Delay_us(US);
	
	SMBUS_SCL_1;
	Delay_us(US);
	
	SMBUS_SDA_0;
	Delay_us(US);
	
	SMBUS_SCL_0;
	Delay_us(US);
}

uint8_t SMBus_SendByte(uint8_t byte) 
{
	int8_t i = 0, j = 0;
	
	//将数据在scl为低电平时转换，在scl为高电平时让传感器读取
	for(i = 7; i >= 0; i--)
	{
		
		SMBUS_SCL_0;
		Delay_us(US);
		
		if ( (byte & (0x80 >> j)) >> i )
		{
			SMBUS_SDA_1;
			Delay_us(US);
		}
		else
		{
			SMBUS_SDA_0;
			Delay_us(US);
		}
			
	
		SMBUS_SCL_1;
		Delay_us(US);
		
		j++;
		
	}

	return WaitACK();
}

void SMBus_SendBit(bit_t ack)
{
	SMBUS_SCL_0;
	Delay_us(US);
	
	if(ack == ACK)
	{
		SMBUS_SDA_0;
		Delay_us(US);
	}
	else
	{
		SMBUS_SDA_1;
		Delay_us(US);
	}
	
	SMBUS_SCL_1;
	Delay_us(US);
		
	SMBUS_SCL_0;
	Delay_us(US);
}

uint8_t WaitACK(void)
{
	//SMBUS_SDA_1;
	uint8_t ack = 2;
	 
	cnt++;
	
	SMBUS_SCL_0;
	SMBUS_SDA_1;
	Delay_us(1);

	
	SMBUS_SCL_1;
	Delay_us(1);
	
	//UART_PRINTF("GPIOB->ODR = %#x\n",GPIOB->ODR);
	ack = SMBUS_READ_SDA();
	if(ack != 0)
	{
		UART_PRINTF("after %d times, ACK = %d\n",cnt, ack);
		return 1;
	}
	
	
	SMBUS_SCL_0;
	Delay_us(1);
	
	return 0;
}

uint8_t SMBus_ReadByte(void)
{
	int8_t i = 0, ack = 1;
	uint8_t data = 0;
	
	for(i = 7; i >= 0; i--)
	{
		SMBUS_SCL_0;
		Delay_us(1);
		
		SMBUS_SCL_1;
		Delay_us(1);
		
		//data |= SMBUS_READ_SDA() << i;	
		if(SMBUS_READ_SDA())
		{
			data |= 1 << i; 
		}	
	}
	
	SMBUS_SCL_0;
	Delay_us(1);
	
	SMBUS_SCL_1;
	Delay_us(1);
	
	ack = SMBUS_READ_SDA();

	if (ack == 1)
	{
		UART_PRINTF("SLAVE NOT RELEASE\n");
		SMBUS_SDA_0;
		Delay_us(US);
	}
	
	SMBUS_SCL_0;
	Delay_us(US);
	

	
	return data;
}

void SMBus_Stop(void)
{
	SMBUS_SDA_0;
	Delay_us(US);
	
	SMBUS_SCL_1;
	Delay_us(US);
	
	SMBUS_SDA_1;
	Delay_us(US);
		
}

int16_t GetSlaveAddress(void)
{
	uint16_t data_low = 0, data_high = 0;
	uint8_t pec = 0;
	uint16_t data = 0;
	int8_t ack = -2;
	
	
	SMBus_Start();
	if(SMBus_SendByte(0xB4) == 0)
	{
		if(SMBus_SendByte(0x2E) == 0)
		{
			SMBus_Start();
			if(SMBus_SendByte(0xB5) == 0)
			{
				data_low = SMBus_ReadByte();
				
				//UART_PRINTF("Slave Address: %#x\n",data);
				data_high =  SMBus_ReadByte();
				pec = SMBus_ReadByte();
				SMBus_Stop();
				
//				UART_PRINTF("data_high = %#x\n",data_high);
				UART_PRINTF("SLAVE PEC = %#x \n", pec);
			}
			else
			{
				UART_PRINTF("Read Command wrong!");
			}
        }
		else
		{
			UART_PRINTF("Write Command Wrong!\n");
		}
	}
	else
	{
		UART_PRINTF("Write Slave Address Wrong!\n");
	}

	
	data = (data_high << 8) | data_low;
	return data;
	
	
}

uint16_t GetTemp(void)
{
	uint16_t data = 0;
	uint16_t data_low = 0, data_high = 0;
	uint8_t pec = 0;
	int8_t ack = -2;

	SMBus_Start();
	
	SMBus_SendByte(0xB4);
	SMBus_SendByte(0x07);
	SMBus_Start();
	SMBus_SendByte(0xB5);
	data_low = SMBus_ReadByte();
	
	data_high = SMBus_ReadByte();
	
	data = (data_high << 8) | data_low;
	
	SMBus_Stop();
	
	return data;
	
}

uint16_t GetEmissivity(void)
{
	uint16_t data_low = 0, data_high = 0, data = 0;
	uint8_t pec = 0;
	
	SMBus_Start();
	
	SMBus_SendByte(0xB4);
	SMBus_SendByte(0x24);
	SMBus_Start();
	SMBus_SendByte(0xB5);
	data_low = SMBus_ReadByte();
	data_high = SMBus_ReadByte();
	
	pec = SMBus_ReadByte();
	SMBus_Stop();
	UART_PRINTF("EMISS PEC %#x\n", pec);
	
	data = data_high << 8 | data_low;
		
	return data;
}


void SetEmissivity(float emissivity)
{
	uint16_t emissivity_hex = (uint16_t)(emissivity * 65535);
	uint8_t i = 0;
	crc_buffer[2] = emissivity_hex & 0xff; //LSByte
	crc_buffer[3] = emissivity_hex >> 8;   //MSByte
	crc_buffer[4] = CRC8(crc_buffer,4);
	
	UART_PRINTF("emissivity hex = %#x\n",emissivity_hex);
	UART_PRINTF("CRC:");
	for(i = 0; i < sizeof(crc_buffer)/sizeof(uint8_t); i++)
	{
		UART_PRINTF("%#x ",crc_buffer[i]);
	}
	UART_PRINTF("\n");
	
	SMBus_Start();
	
	for(i = 0; i < sizeof(crc_buffer)/sizeof(uint8_t); i++)
	{
		SMBus_SendByte(crc_buffer[i]);
	}
	
	SMBus_Stop();
}

