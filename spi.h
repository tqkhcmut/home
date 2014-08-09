#ifndef __SPI_H
#define __SPI_H

#ifdef __cplusplus
extern "C" {
#endif
	
void	SPI2_Init(void);

unsigned char	SPI2_ReadWrite(unsigned char writedat);
// void waitspi(void);

#ifdef __cplusplus
}
#endif

#endif
