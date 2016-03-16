// spi.c

#include "decls.h"

// Hold this semaphore to call spiSendReceive
xSemaphoreHandle spiMutex;

// Start the SPI subsystem
void startSpi(void) {
	spiMutex = xSemaphoreCreateMutex();
	ASSERT_BLINK(spiMutex, 4, 1);
}

#define SPI_TIMEOUT_MS 1000

// Executes fn while holding the SPI mutex
void spiWithMutex(void (*fn)(void)) {
	// Take the mutex
	if (!xSemaphoreTake(spiMutex, 1000)) {
		fatalBlink(4, 2);
	}
	
	// Do the thing
	fn();
	
	// Return Mutex
	if (!xSemaphoreGive(spiMutex)) {
		fatalBlink(4, 3);
	}
}

// Send a datum, receive a datum.
// Channel number must be encoded in bits 19-16
// Acquire semaphore before using
// TODO: Work out DMA for longer send chains
uint16_t spiSendReceive(uint32_t datum) {
	portTickType timeout = xTaskGetTickCount() + MS_TO_TICKS(SPI_TIMEOUT_MS);
	// Empty receive data register
	//SPI0->SPI_RDR;
	
	// Transmit datum
	SPI0->SPI_TDR = datum;
	
	// Busy-ish waiting. At 10MHz, will take 1.6uS to send 16bits
	// TODO: Use an interrupt + queue instead.
	while (((SPI0->SPI_SR & SPI_SR_RDRF) == 0) && (xTaskGetTickCount() < timeout)) {
		portYIELD();
	}
	
	if (xTaskGetTickCount() >= timeout) {
		errorBlink(4, 1);
	}
	return SPI0->SPI_RDR;
}