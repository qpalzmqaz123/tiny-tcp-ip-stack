#include "spi_enc28j60.h"
#include "spi.h"
																				  
void SPI_Enc28j60_Init(void)
{
	spiInit();
}

unsigned char	SPI1_ReadWrite(unsigned char writedat)
{
	return spiSR(writedat);
}

