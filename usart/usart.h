#ifndef _usart_h_
#define _usart_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"

typedef enum 
{
	BIN = 0,
	OCT,
	DEC,
	HEX
} BYTE_FORMAT;

#define USART_BUFFER_SIZE 	32

void USART1_init(void);
void USART1_sendChar(char c);
void USART1_sendStr(char Str[]);
void USART1_sendNum(long num);
void USART1_sendFloat(float num);
void USART1_sendByte(uint8_t b, BYTE_FORMAT f);

#ifdef __cplusplus
}
#endif
	
#endif

