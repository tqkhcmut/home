#ifndef _usart_h_
#define _usart_h_

//#define USE_USART1

#include "stm32f10x.h"

typedef enum 
{
	BIN = 0,
	OCT,
	DEC,
	HEX
} BYTE_FORMAT;

#define USE_USART1
#define USART_BUFFER_SIZE 	32
#define USART_BUFFERED

#ifdef USART_BUFFERED
void USART1_init(void);
void USART1_sendChar(char c);
void USART1_sendStr(char Str[]);
void USART1_sendNum(long num);
void USART1_sendFloat(float num);
void USART1_sendByte(uint8_t b, BYTE_FORMAT f);
#endif

#endif

