// spi.c


// Start the SPI subsystem
void startSpi(void) {
	GLOBAL_STATE.usingSpi = xSemaphoreCreateMutex();
	ASSERT_BLINK(GLOBAL_STATE.usingSpi != NULL, 4, 1);
}


// Force SPI_RDR to be read
static __attribute__((optimize("O0"))) inline int32_t forceReadRDR(void) {
	return SPI0->SPI_RDR;
}

#define SPI_TIMEOUT_MS 1000

// Send a datum, receive a datum.
// Channel number must be encoded in bits 19-16
// Acquire semaphore before using
uint32_t spiSendReceive(uint32_t datum) {
	portTickType timeout = xTaskGetTickCount() + MS_TO_TICKS(SPI_TIMEOUT_MS);
	// Empty receive data register
	forceReadRDR();
	SPI0->SPI_TDR = datum;
	
	// Busy-ish waiting. 
	// TODO: use an interrupt
	while (SPI0->SPI_SR & SPI_SR_RDRF && xTaskGetTickCount() < timeout) {
		portYIELD();
	}
	return SPI0->SPI_RDR;
}