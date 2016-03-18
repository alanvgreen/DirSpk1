// screen.c
//
// Routines for manipulating the display (RA8875 based)
//
#include "decls.h"

// How long to wait for SPI
#define LCD_SPI_WAIT_MS 1000

#define ACQUIRE_SPI_MUTEX \
	ASSERT_BLINK( \
		xSemaphoreTake(spiMutex, MS_TO_TICKS(LCD_SPI_WAIT_MS)), \
		8, 2)
#define RELEASE_SPI_MUTEX \
	ASSERT_BLINK(xSemaphoreGive(spiMutex), 8, 3)

// Queue for screen commands
xQueueHandle screenQueue;

// Basic LCD driving commands
static uint8_t lcdStatusRead(void) {
	return spiSendReceive(SCREEN_TO_TDR(0xc000)) & 0xff;
}

// Set register r = v
static void lcdSetReg(uint8_t r, uint8_t v) {
	spiSendReceive(SCREEN_TO_TDR(0x8000 + r)); // command write
	spiSendReceive(SCREEN_TO_TDR(0x0000 + v)); // data write
}

// set register r = v & 0xff, register r+1 = v >> 8
static void lcdSetRegPair(uint8_t r, uint16_t v) {
	lcdSetReg(r, v);
	lcdSetReg(r + 1, v >> 8);
}

// Set Active Window area. x0 < x1, y0 < y1
static void lcdSetActiveWindow(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1) {
	lcdSetRegPair(0x30, x0); // HSAW0, 1
	lcdSetRegPair(0x32, y0); // VSAW0, 1
	lcdSetRegPair(0x34, x1); // HEAW0, 1
	lcdSetRegPair(0x36, y1); // VEAW0, 1
}

static void lcdWaitNotBusy(void) {
	// Wait until all busy status flags are zero
	while (lcdStatusRead() && 0xc1) {
		RELEASE_SPI_MUTEX;
		vTaskDelay(0);
		ACQUIRE_SPI_MUTEX;
	}
}

// Clear the screen
static void lcdClear(void) {
	lcdSetReg(0x8e, 0x80); // Clear screen
	lcdWaitNotBusy();
}

// Initialize the LCD panel
static void screenInitLcd(void) {
	// Set PLL to 60MHz in two steps, allowing 1ms for stabilization in between
	// (Datasheet says only 100us needed, but we can more easily time 1ms)
	lcdSetReg(0x88, 0x0c);
	RELEASE_SPI_MUTEX;
	vTaskDelay(MS_TO_TICKS(1)); 
	ACQUIRE_SPI_MUTEX;
	lcdSetReg(0x89, 0x02);
	RELEASE_SPI_MUTEX;
	vTaskDelay(MS_TO_TICKS(1));
	ACQUIRE_SPI_MUTEX;
	
	// TODO(avg): set SPI up to 10MHz here.
	// Actually 40MHz is OK for writes, but needs testing for signal
	
	
	// TODO(avg) think about whether 2 windows @8bpp would be more useful
	// than one window @16bpp 
	lcdSetReg(0x10, 0x0c); // Set 16bpp 
	lcdSetReg(0x04, 0x81); // Pixel clock settings
	RELEASE_SPI_MUTEX;
	vTaskDelay(MS_TO_TICKS(1)); // Example code has delay here
	ACQUIRE_SPI_MUTEX;
	
	// Horizontal settings
	lcdSetReg(0x14, 0x63); // width = 800px
	lcdSetReg(0x15, 0x00); // Horizontal Non-Display Period Fine Tuning
	lcdSetReg(0x16, 0x03); // Horiz. Nond display period = 34px
	lcdSetReg(0x17, 0x03); // HSYNC start = 32px
	lcdSetReg(0x18, 0x0b); // HSYNC pulse width and polarity
	
	// Vertical settings
	lcdSetReg(0x19, 0xdf); // Height = 0x1df + 1 = 480 
	lcdSetReg(0x1a, 0x01);
	lcdSetReg(0x1b, 0x20); // Vertical non-display area  = 33px
	lcdSetReg(0x1c, 0x00);
	lcdSetReg(0x1d, 0x16); // VSYNC start = 23px
	lcdSetReg(0x1e, 0x00);
	lcdSetReg(0x1f, 0x01); // VSYNC pulse width = 2
	
	lcdSetActiveWindow(0, 0, 799, 479);
	
	// Set up backlight and turn on display
	lcdSetReg(0x8a, 0x80); // Enable PWM
	lcdSetReg(0x8a, 0x81); // Enable PWM, set freq = CLK/2
	lcdSetReg(0x8b, 0xff); // Max brightness
	lcdSetReg(0x01, 0x80); // On
	
	lcdClear();
}

// Runs everything that sends commands to the screen
static void screenTask(void *pvParameters) {
	while (1) {
		ScreenCommand cmd;
		if (!xQueueReceive(screenQueue, &cmd, 997)) {
			continue;
		}
		
		// Assume that all commands take the SPI mutex
		ACQUIRE_SPI_MUTEX;
		if (cmd.type== SCREEN_INIT) {
			screenInitLcd();
		} else {
			// Unknown type
			fatalBlink(5, 2);
		}
		RELEASE_SPI_MUTEX;
	}
}

// Start the screen
void startScreen(void) {	
	// Screen queue is deep enough for some slop.
	screenQueue = xQueueCreate(10, sizeof(ScreenCommand));
	portBASE_TYPE t = xTaskCreate(
		screenTask,
		USTR("SCRN"),
		configMINIMAL_STACK_SIZE * 4,
		NULL,
		tskIDLE_PRIORITY + 2, // More important than the CLI
		NULL);
	ASSERT_BLINK(t, 7, 1);
}
