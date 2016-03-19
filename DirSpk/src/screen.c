// screen.c
//
// Routines for manipulating the display (RA8875 based)
//
#include "decls.h"
#include "colors.h"

// Rainbow colors for special effects
static uint32_t const rainbow_cols[] = {
	COL_red, COL_crimson, COL_orange, COL_yellow, COL_yellowgreen,
	COL_green, COL_navy, COL_blue, COL_purple, COL_fuchsia,
};
uint16_t const rainbow_size = 10;

// Strings
static const char *MSG_DIRSPK1 = "DirSpk1";

// How long to wait for SPI
#define LCD_SPI_WAIT_MS 1000

#define ACQUIRE_SPI_MUTEX \
	ASSERT_BLINK( \
		xSemaphoreTake(spiMutex, MS_TO_TICKS(LCD_SPI_WAIT_MS)), \
		2, 5)
#define RELEASE_SPI_MUTEX \
	ASSERT_BLINK(xSemaphoreGive(spiMutex), 3, 5)

// Queue for screen commands
static xQueueHandle screenQueue;

// Default wait for screen queue
#define SCREEN_QUEUE_WAIT_MS 1000
void screenSendCommand(ScreenCommand *command) {
	if (!xQueueSendToBack(screenQueue, command, SCREEN_QUEUE_WAIT_MS)) {
		fatalBlink(4, 5);
	}
}

// Basic LCD driving commands
static uint8_t lcdStatusRead(void) {
	return spiSendReceive(SCREEN_TO_TDR(0xc000)) & 0xff;
}

// Wait until all busy status flags are zero
static void lcdWaitNotBusy(void) {
	while (lcdStatusRead() && 0xc1) {
		RELEASE_SPI_MUTEX;
		vTaskDelay(0);
		ACQUIRE_SPI_MUTEX;
	}
}

// Write a bunch of 8 bit values to memory
static void lcdWriteMem(uint8_t const *mem, uint16_t len) {
	lcdWaitNotBusy();
	spiSendReceive(SCREEN_TO_TDR(0x8002)); // Command register 2
	for (uint16_t i = 0; i < len; i++) {
		spiSendReceive(SCREEN_TO_TDR(0xff & *(mem++)));
		lcdWaitNotBusy();
	}
}

// Write a bunch of 8-bit chars to memory
static void lcdWriteString(char const *s) {
	lcdWriteMem((uint8_t const *)s, strlen(s));
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

// Set 16 bit values into consecutive pairs of registers
static void lcdSetRegXY(uint8_t r, uint16_t x, uint16_t y) {
	lcdSetRegPair(r, x);
	lcdSetRegPair(r+2, y);
}

// Set Active Window area. x0 < x1, y0 < y1
static void lcdSetActiveWindow(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1) {
	lcdSetRegXY(0x30, x0, y0); // {H,V}SAW{0,1}
	lcdSetRegXY(0x34, x1, y1); // {H,V}EAW{0,1}
}

// Set font write params
static void lcdSetTextParams(uint16_t x, uint16_t y, uint8_t size, uint8_t spacing) {
	lcdSetRegXY(0x2a, x, y);
	lcdSetReg(0x22, 0x40 + ((size & 3) << 2) + (size & 3)); // 0x40 = Always transparent
	lcdSetReg(0x2e, spacing);
	lcdSetReg(0x40, 0x80); // Move into text mode
}

// Set color into three consecutive registers
// Color is 24bit RGB, but only 16 or 8 bits will be used, depending on mode
static void lcdSetColor(uint8_t r, uint32_t color) {
	// TODO(avg): implement 8bpp mode
	lcdSetReg(r+0, (0xff0000 & color) >> (16 + 3)); // 5 bits of red
	lcdSetReg(r+1, (0x00ff00 & color) >> (8 + 2)); // 6 bits of green
	lcdSetReg(r+2, (0x0000ff & color) >> 3); // 5 bits of blue
}

// Set background color
static inline void lcdSetBackgroundColor(uint32_t color) {
	lcdSetColor(0x60, color);
}

// Set foreground color
static inline void lcdSetForegroundColor(uint32_t color) {
	lcdSetColor(0x63, color);
}

// Make a rounded-rectangle. The internal space will be set to
// the background color
static inline void lcdRoundedRect(
uint16_t x0, uint16_t y0, 
uint16_t x1, uint16_t y1, 
uint8_t radius, int16_t width,
uint32_t borderColor, int32_t fillColor) {
	lcdSetForegroundColor(borderColor);
	lcdSetRegXY(0x91, x0, y0);
	lcdSetRegXY(0x95, x1, y1);
	lcdSetRegXY(0xa1, radius, radius);
	lcdSetReg(0xa0, 0xe0); // Draw square of circle corner, filled
	lcdWaitNotBusy();
	
	lcdSetForegroundColor(fillColor);
	lcdSetRegXY(0x91, x0 + width, y0 + width);
	lcdSetRegXY(0x95, x1 - width, y1 - width);
	lcdSetRegXY(0xa1, radius - width, radius - width);
	lcdSetReg(0xa0, 0xe0); // Draw square of circle corner, filled
	lcdWaitNotBusy();
}

// Clear the screen
static void lcdClear(uint32_t color) {
	lcdSetBackgroundColor(color);
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
	lcdSetReg(0x16, 0x03); // Horiz. Nondisplay period = 34px
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
	
	// Set External Arial font by default
	lcdSetReg(0x05, 0x28); // external timing magic
	lcdSetReg(0x06, 0x03); // external timing magic
	lcdSetReg(0x2e, 0x01); // single pixel between chars
	lcdSetReg(0x2f, 0x11); // ROM type 0, Arial
	lcdSetReg(0x21, 0x20); // Enable external font
	
	// Just clearing the cobwebs - seems to help avoid missing text
	lcdWriteString("    "); 
}

// Show startup page
static void screenShowStartup(void) {
	lcdClear(0x404040);
	
	lcdRoundedRect(50, 50, 749, 439, 15, 8, COL_silver, COL_black);
	
	// Make rainbow logo 
	for (int i = rainbow_size - 1; i >= 0; i--) {
		uint16_t x = 164 + i * 24;
		uint16_t y = 100 + i * 20;
		// Make an outline
		lcdSetForegroundColor(COL_black);
		for (int16_t xd = -2; xd <= 2; xd += 4) {
			for (int16_t yd = -2; yd <= 2; yd += 4) {
				lcdSetTextParams(xd + x, yd + y, 3, 3);
				lcdWriteString(MSG_DIRSPK1);
			}
		}
		// Write string
		lcdSetTextParams(x, y, 3, 3);
		lcdSetForegroundColor(rainbow_cols[i]);
		lcdWriteString(MSG_DIRSPK1);
	}
	
	// Write subtitle
	lcdSetTextParams(295, 360, 0, 1);
	lcdSetForegroundColor(COL_silver);
	lcdWriteString("for ");
	lcdSetForegroundColor(COL_white);
	lcdWriteString("mvg ");
	lcdSetForegroundColor(COL_silver);
	lcdWriteString("with love from avg");
}

// Turn the screen off - for debugging
static void screenTurnOff(void) {
	lcdSetReg(0x01, 0x00); // Off
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
		if (cmd.type == SCREEN_INIT) {
			screenInitLcd();
		} else if (cmd.type == SCREEN_OFF) {
			screenTurnOff();
		} else if (cmd.type == SCREEN_STARTUP) {
			screenShowStartup();
		} else {
			// Unknown type - use error rather than fatal as commands
			// can be sent via CLI
			errorBlink(5, 5);
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
	ASSERT_BLINK(t, 1, 5);
}
