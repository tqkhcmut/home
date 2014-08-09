#ifndef delay_h
#define delay_h

#ifdef __cplusplus
 extern "C" {
#endif
	 
#ifndef USE_SYSTICK
#define USE_SYSTICK
#else 
#error "Systick have been used!"
#endif

#include "stm32f10x.h"
	 #include "assistant.h"

// // if you want to use nonblocking delay functions, mask this 1
// // else mask this 0
// #ifndef DELAY_NONBLOCKING
// #define DELAY_NONBLOCKING 0
// #endif

// #if defined(DELAY_NONBLOCKING)

// // Not available now (^_^)
// #else

void delay_init(void);
void _delay_ms(unsigned int time); // delay for miliseconds
void _delay_us(unsigned int time); // delay for microseconds
unsigned int millis(void);	// return current miliseconds
void setMillis(unsigned int time); // delay for microseconds

// add following function on to Systick interrupt's handle
void delay_routine(void);
// #endif

#ifdef __cplusplus
}
#endif

#endif
