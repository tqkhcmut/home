#include "delay.h"

volatile unsigned int delayCounter = 0;
volatile unsigned int millisCounter = 0;
//volatile unsigned int millisCount = 0;

void delay_init(void)
{
	if (SysTick_Config(SystemCoreClock / 1000)) // 1 ms
	{ 
		/* Capture error */ 
		while (1);
	}
}

void _delay_ms(unsigned int time) // delay for miliseconds
{
	delayCounter = time;
	while(delayCounter);
}

void _delay_us(unsigned int time) // delay for microseconds
{	
	unsigned int i = (time * 72)/5; // because cpu clock is 72MHz. I don't know why divide by 5 :D
	while(i--);
}

unsigned int millis(void)
{
	return millisCounter;
}

void setMillis(unsigned int time)
{
	millisCounter = time;
}


void delay_routine(void)
{
	if (delayCounter)
		delayCounter--;
	millisCounter++;
}
