#ifndef _ASSISTANT_H_
#define _ASSISTANT_H_

#ifdef __cplusplus
extern "C" {
#endif
	
	#include "stm32f10x.h"
	#include "usart/usart.h"
	#include "delay/delay.h"

	void RCC_Configuration(void);
	void GPIO_Configuration(void);
	void NVIC_Configuration(void);

#ifdef __cplusplus
}
#endif

#endif
