// ui.c
//
// The UI handling task and code

#include "decls.h"

// UiQueue for events from encoders, touch, etc. Contains 
xQueueHandle uiQueue;

// Gain for channels zero and 1. Written by: UI task.
volatile uint16_t gain0, gain1;

// Whether UIQueue has ever been full. Set only. Written by: any sender to uiQueue.
volatile bool uiQueueFullFlag;

// Timer tick
static void uiTickCallback(xTimerHandle pxTimer) {
	UiEvent event = {
		.type = UI_TICK,
	};
	if (!xQueueSendToBack(uiQueue, &event, 0)) {
		uiQueueFullFlag = true;
	}
}

// Update gain to pot. Execute while holding the SPI mutex. 
static void updateGain(void) {
	uint16_t vol = gain0;
	// Set reg0
	spiSendReceive((0 << 12) + vol);
	// Set reg1
	spiSendReceive((1 << 12) + vol);
}


// Handled encoder moves
static void uiHandleEncoderEvent(EncoderMove *p) {
	// Set both pots to vol0
	if (p->num == 3) {
		int16_t val = gain0;
		val += (p->dir == ENC_CW) ? 4 : -4;
		val = max(min(val, 0x100), 0);
		gain0 = gain1 = val;
		spiWithMutex(updateGain);
	}
	
	// TODO: master + fade
	// TODO: acceleration
	// TODO: update NV pot after changes stop for a while
}

// Handle the UI_TICK event
static void uiHandleTickEvent(void) {
	// TODO(avg): add real data
	ScreenCommand input = {
		.type = SCREEN_INPUT,
		.leftLevel = 30,
		.rightLevel = 70,
		.gain = gain0,
		.fade = 50,
	};
	screenSendCommand(&input);
}

// Fetch volume from pot. Execute while holding the SPI mutex.
static void getVolume(void) {
	// Read r0
	gain0 = spiSendReceive(0x0c00) & 0x1ff;

	// Read r1
	gain1 = spiSendReceive(0x1c00) & 0x1ff;	
}


// The ui coordinator task.
static void uiTask(void *pvParameters) {
	
	// Initialize the UI global state
	spiWithMutex(getVolume);
	
	// Show startup screen
	ScreenCommand init = {.type = SCREEN_INIT};
	screenSendCommand(&init);
	ScreenCommand startup = {.type = SCREEN_STARTUP};
	screenSendCommand(&startup);
	vTaskDelay(MS_TO_TICKS(700));
	
	// Get an event, deal with it
	while (1) {
		UiEvent event;
		if (!xQueueReceive(uiQueue, &event, 1001)) {
			continue;
		}
		if (event.type == UI_ENCODER) {
			uiHandleEncoderEvent(&event.encMove);
		} else if (event.type == UI_TICK) {
			uiHandleTickEvent();
		} else {
			// Unknown type
			fatalBlink(1, 3);
		}
	}
}

// Start the UI
void startUi(void) {
	// 10 deep is a little wasteful of UI, and might to lead to sloppy, unresponsive UI.
	// Let's see.
	uiQueue = xQueueCreate(10, sizeof(UiEvent));
	
	xTimerHandle timer = xTimerCreate("UI_TICK", MS_TO_TICKS(20), true, NULL, uiTickCallback);
	xTimerStart(timer, 0);
	
	portBASE_TYPE task = xTaskCreate(
		uiTask,
		USTR("UI"),
		configMINIMAL_STACK_SIZE * 4,
		NULL,
		tskIDLE_PRIORITY + 2, // More important than the CLI
		NULL);
	ASSERT_BLINK(task, 2, 3);
}