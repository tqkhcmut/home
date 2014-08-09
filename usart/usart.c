#include "usart.h"

#ifdef USART_BUFFERED
volatile uint8_t TxBuffer[USART_BUFFER_SIZE];
volatile uint8_t RxBuffer[USART_BUFFER_SIZE];
volatile int16_t RxCounter = -1;
volatile int16_t TxCounter = -1;

void USART1_init(void)
{
	// USART init
	USART_InitTypeDef USART_InitStructure;
	
	USART_InitStructure.USART_BaudRate            = 115200;
	USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits            = USART_StopBits_1;
	USART_InitStructure.USART_Parity              = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
	
	USART_Init(USART1, &USART_InitStructure);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
 	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
	USART_Cmd(USART1, ENABLE);
}

void USART1_sendChar(char c)
{
	while(TxCounter >= 0); // wait
	TxBuffer[++TxCounter] = c;
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
}

void USART1_sendStr(char Str[])
{
	int16_t i = 0;
	while(TxCounter >= 0); // wait
	while (Str[i])
	{
		i++;
	}
	i--;
	TxCounter = i;
	for (i = i; i >= 0; i--)
		TxBuffer[TxCounter-i] = Str[i];
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
}

void USART1_sendNum(long num)
{
	char negative = ' ';
	while(TxCounter >= 0); // wait
// 	TxBuffer[++TxCounter] = '\n';
	if (num < 0)
	{
		negative = '-';
		num = 0 - num;
	}
	if (num == 0)
	{
		TxBuffer[++TxCounter] = '0';
	}
	while (num != 0)
	{
		TxBuffer[++TxCounter] = num%10+0x30;
		num = num / 10;
	}
	if (negative == '-')
		TxBuffer[++TxCounter] = '-';
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
}

void USART1_sendFloat(float num)
{
	int __int = (int) num;
	USART1_sendNum(__int);
	USART1_sendChar('.');
	__int = (int)((num-__int)*100);
	if (__int < 0)
		__int = 0;
	USART1_sendNum(__int);
}

void USART1_sendByte(uint8_t b, BYTE_FORMAT f)
{
	uint8_t bitMask = 1;
	uint8_t i;
	switch (f)
	{
		case BIN:
			for (i = 8; i > 0; i--)
			{
				USART1_sendChar((b&(bitMask << i)) ? '1' : '0');
			}
			break;
		case OCT:
			break;
		case DEC:
			USART1_sendNum(b);
			break;
		case HEX:
			if(b/16 < 10){
				USART1_sendChar(0x30 + b/16);
			}else{
				USART1_sendChar(0x37 + b/16);
			}

			if(b%16 < 10){
				USART1_sendChar(0x30 + b%16);
			}else{
				USART1_sendChar(0x37 + b%16);
			}
			break;
		default:
			break;
	}
}

void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		RxBuffer[RxCounter--] = USART_ReceiveData(USART1);
		if (RxCounter < 0)
		{
			USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
		}
	}
	if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
	{
		USART_SendData(USART1, TxBuffer[TxCounter--]);
		if (TxCounter < 0)
		{
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
		}
	}
	// Echo code
}
#endif
