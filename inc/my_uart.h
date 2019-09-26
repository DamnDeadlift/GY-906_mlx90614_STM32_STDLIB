#ifndef __MY_UART_H__
#define __MY_UART_H__

#include <stdio.h>
#include "stm32f10x.h"


#define UART_PRINTF printf 

int fputc(int ch, FILE *f);
void UART_Config(uint32_t baud);

#endif