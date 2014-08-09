#ifndef converter_h
#define converter_h

#include "usart/usart.h"

#define END_STRING '\0'

extern char converted[16];

char * Num_toString(int num);
char * Byte_toString(unsigned char b, BYTE_FORMAT f);
char * Float_toString(float f);

#endif
