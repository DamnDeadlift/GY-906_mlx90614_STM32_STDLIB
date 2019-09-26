#include "stm32f10x.h"
#include "mlx90614.h"
#include "my_uart.h"

volatile uint16_t count = 0;

void GPIO_Config(void);
void Basic_Timer_Config(void);
void Delay_us(uint16_t us);

int main(void)
{
	uint8_t i = 0;
	uint16_t slave_address = 0, newaddress = 0;
	uint16_t temp = 0;
	uint16_t emissivity_hex = 0;
	float emissivity = 0.0;
	float temperture = 0.0;
	
	GPIO_Config();
	UART_Config(115200);
	Basic_Timer_Config();
	
	UART_PRINTF("START!!!\n");
	
	slave_address = GetSlaveAddress();
	UART_PRINTF("SLAVE ADDRESS = %#x\n", slave_address);
	
	newaddress = 0x24;
	if(slave_address != newaddress)
	{
		UART_PRINTF("start set address\n");
		SetSlaveAddress(0x00);
		Delay_us(6000);
		
		if(SetSlaveAddress(newaddress) != 0)  //set address failed
		{
			slave_address = GetSlaveAddress();
		}
	}
		
	
	Delay_us(6000);
	slave_address = GetSlaveAddress();
	UART_PRINTF("SLAVE ADDRESS = %#x\n", slave_address);
	
	
	SetEmissivity(slave_address, 0);
	Delay_us(6000);
	
	SetEmissivity(slave_address, 0.9);
	Delay_us(6000);
	
	emissivity_hex = GetEmissivity(slave_address);
	UART_PRINTF("emissivity hex = %#x\n", emissivity_hex);
	emissivity = (float)emissivity_hex / (float)65535;
	UART_PRINTF("emissivity = %.1f\n", emissivity);
	
	while(1)
	{
		temp = GetTemp(slave_address);
		temperture = temp * 0.02 - 273.15;
		UART_PRINTF("temp = %.2f\n", temperture);	
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

void Delay_us(uint16_t us)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	while(count != us);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, DISABLE);
	count = 0;
}