#ifndef __SPI_H
#define __SPI_H

#define SPI_CS_L GPIOA->BSRRH = (1 << 4)
#define SPI_CS_H GPIOA->BSRRL = (1 << 4) 

#define SPI_CLK_L GPIOA->BSRRH = (1 << 5)
#define SPI_CLK_H GPIOA->BSRRL = (1 << 5)
 
#define SPI_MISO_IN (GPIOA->IDR & (1 << 6))

#define SPI_MOSI_L GPIOA->BSRRH = (1 << 7)
#define SPI_MOSI_H GPIOA->BSRRL = (1 << 7) 

void spiInit(void);
unsigned char spiSR(unsigned char data);
void spiSend(unsigned char data);
unsigned char spiReceive(void);

#endif
