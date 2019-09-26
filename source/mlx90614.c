#include "mlx90614.h"
#include "my_uart.h"

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

uint8_t WaitACK(void)
{
	uint8_t ack = 2;
	
	SMBUS_SCL_0;
	SMBUS_SDA_1;
	Delay_us(1);
	
	SMBUS_SCL_1;
	Delay_us(1);
	
	ack = SMBUS_READ_SDA();
	if(ack != 0)
	{
		return 1;
	}
	
	SMBUS_SCL_0;
	Delay_us(1);
	
	return 0;
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

uint16_t GetSlaveAddress(void)
{
	uint16_t data_low = 0, data_high = 0, data = 0;
	uint8_t pec = 0;
	uint8_t ack = 1;
	
	SMBus_Start();
	if(SMBus_SendByte(00) == 0)
	{
		if(SMBus_SendByte(0x2E) == 0)
		{
			SMBus_Start();
			if(SMBus_SendByte(01) == 0)
			{
				data_low = SMBus_ReadByte();
			
				data_high =  SMBus_ReadByte();
				pec = SMBus_ReadByte();

				SMBus_Stop();
				
				UART_PRINTF("SLAVE PEC = %#x \n", pec);
			}
			else
			{
				UART_PRINTF("Read Command wrong!");
                return 0;
			}
        }
		else
		{
			UART_PRINTF("Write Command Wrong!\n");
            return 0;
		}
	}
	else
	{
		UART_PRINTF("Write Slave Address Wrong!\n");
        return 0;
	}

	data = data_low;  //Only LSByte
	return data;
	
}

uint16_t GetTemp(uint8_t slave_address)
{
	uint16_t data = 0;
	uint16_t data_low = 0, data_high = 0;
	uint8_t pec = 0;
	int8_t ack = -2;

	SMBus_Start();
	
	SMBus_SendByte(slave_address);
	SMBus_SendByte(0x07);
	SMBus_Start();
	SMBus_SendByte(slave_address + 1);

	data_low = SMBus_ReadByte();
	data_high = SMBus_ReadByte();
	
    SMBus_Stop();

	data = (data_high << 8) | data_low;

	return data;
	
}

uint16_t GetEmissivity(uint8_t slave_address)
{
	uint16_t data_low = 0, data_high = 0, data = 0;
	uint8_t pec = 0;
	
	SMBus_Start();
	
	SMBus_SendByte(slave_address);
	SMBus_SendByte(0x24);
	SMBus_Start();
	SMBus_SendByte(slave_address + 1);

	data_low = SMBus_ReadByte();
	data_high = SMBus_ReadByte();
	
	pec = SMBus_ReadByte();
	SMBus_Stop();

	UART_PRINTF("EMISS PEC %#x\n", pec);
	
	data = data_high << 8 | data_low;
		
	return data;
}

void SetEmissivity(uint8_t slave_address, float emissivity)
{
	uint16_t emissivity_hex = (uint16_t)(emissivity * 65535);
	uint8_t i = 0;
    crc_buffer[1] = slave_address;
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
	
    //
	SMBus_Start();
	
	for(i = 0; i < sizeof(crc_buffer)/sizeof(uint8_t); i++)
	{
		SMBus_SendByte(crc_buffer[i]);
	}
	
	SMBus_Stop();
}