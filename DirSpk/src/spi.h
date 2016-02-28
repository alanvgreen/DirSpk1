// spi.h

// Routines for accessing the SPI peripheral
#ifndef SPI_H_
#define SPI_H_

// Start the SPI subsystem
void startSpi(void);

// Executes fn while holding the SPI mutex
void spiExclusive(void (*fn)(void));

// Send a datum, receive a datum.
// Channel number must be encoded in bits 19-16
// Acquire semaphore before using 
uint32_t spiSendReceive(uint32_t datum);

// TODO: for sanity's sake, work out how to use DMA for this peripheral
#endif /* SPI_H_ */