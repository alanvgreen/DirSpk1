// ui.c
//
// The UI handling task and code

#include <asf.h>
#include "spi.h"
#include "state.h"
#include "ui.h"
#include "util.h"

// Update gain to pot. Execute while holding the SPI mutex. 
static void updateGain(void) {
	uint16_t vol = GLOBAL_STATE.gain0;
	// Set reg0
	spiSendReceive((0 << 12) + vol);
	// Set reg1
	spiSendReceive((1 << 12) + vol);
}


// Handled encoder moves
static void uiHandleEncoderEvent(EncoderMove *p) {
	// Set both pots to vol0
	if (p->num == 3) {
		int16_t vol = GLOBAL_STATE.gain0;
		vol += (p->dir == ENC_CW) ? 4 : -4;
		vol = max(min(vol, 0x100), 0);
		GLOBAL_STATE.gain0 = GLOBAL_STATE.gain1 = vol;
		spiExclusive(updateGain);
	}
	
	// TODO: master + fade
	// TODO: acceleration
	// TODO: update NV pot after changes stop for a while
}

// Get volume from pot. Execute while holding the SPI mutex.
static void getVolume(void) {
	// Read r0
	GLOBAL_STATE.gain0 = spiSendReceive(0x0c00) & 0x1ff;

	// Read r1
	GLOBAL_STATE.gain1 = spiSendReceive(0x1c00) & 0x1ff;	
}
// The console task.
static void uiTask(void *pvParameters) {
	
	// Initialize the UI global state
	spiExclusive(getVolume);
	
	// Get an event, deal with it
	while (1) {
		UiEvent event;
		if (!xQueueReceive(GLOBAL_STATE.uiQueue, &event, 1001)) {
			continue;
		}
		if (event.type == UI_ENCODER) {
			uiHandleEncoderEvent(&event.encMove);
		} else {
			// Unknown type
			fatalBlink(5, 2);
		}
	}
}

// Start the UI
void startUi(void) {
	// 10 deep is a little wasteful of UI, and might to lead to sloppy, unresponsive UI.
	// Let's see.
	GLOBAL_STATE.uiQueue = xQueueCreate(10, sizeof(UiEvent));
	
	portBASE_TYPE t = xTaskCreate(
		uiTask,
		USTR("UI"),
		configMINIMAL_STACK_SIZE * 4,
		NULL,
		tskIDLE_PRIORITY + 2, // More important than the CLI
		NULL);
	ASSERT_BLINK(t, 5, 1);
}