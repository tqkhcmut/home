#include "converter.h"

char converted[16];

unsigned char i = 0;

char * Num_toString(int num)
{
	unsigned int tmp = 100000000;
	i = 0;
	
	if (num == 0)
	{
		converted[i++] = '0';
		converted[i] = END_STRING;
		return converted;
	}
	
	if (num < 0)
	{
		converted[i++] = '-';
		num = 0 - num;
	}
	
	while (tmp != 0)
	{
		if (num/tmp != 0)
		{
			converted[i++] = (num/tmp)%10 + '0';
		}
		tmp = tmp/10;
	}
	converted[i] = END_STRING;
	return converted;
}

char * Byte_toString(unsigned char b, BYTE_FORMAT f)
{
	unsigned char bitMask = 1;
	unsigned char pos;
	i = 0;
	
	switch (f)
	{
		case BIN:
			for (pos = 8; pos > 0; pos--)
			{
				converted[i++] = ((b&(bitMask << pos)) ? '1' : '0');
			}
			break;
		case OCT:
			break;
		case DEC:
			return Num_toString(b);
//			break;
		case HEX:
			converted[i++] = '0';
			converted[i++] = 'x';
		
			if(b/16 < 10){
				converted[i++] = (0x30 + b/16);
			}else{
				converted[i++] = (0x37 + b/16);
			}

			if(b%16 < 10){
				converted[i++] = (0x30 + b%16);
			}else{
				converted[i++] = (0x37 + b%16);
			}
			break;
		default:
			break;
	}
	converted[i] = END_STRING;
	return converted;
}

char * Float_toString(float f)
{
	unsigned int tmp = 100000000;
	int __int = (int) f;
	i = 0;
	
	if (__int < 0)
	{
		converted[i++] = '-';
		__int = 0 - __int;
	}
	if (__int == 0)
	{
		converted[i++] = '0';
	}
	while (tmp != 0)
	{
		if (__int/tmp != 0)
		{
			converted[i++] = (__int/tmp)%10 + '0';
		}
		tmp = tmp/10;
	}
	
	converted[i++] = ('.');
	
	__int = (int)(f*100 - __int*100);
	
	if (__int < 0)
		__int = 0;
		
	if (__int == 0)
	{
		converted[i++] = '0';
		converted[i] = END_STRING;
		return converted;
	}
	
	tmp = 10;
	
	while (tmp != 0)
	{
		converted[i++] = (__int/tmp)%10 + '0';
		tmp = tmp/10;
	}
	converted[i] = END_STRING;
	
	return converted;
}
