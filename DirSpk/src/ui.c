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

// Internal state variables

// Overall UI Mode
static enum {
	ModeSplash,
	ModeGenerate,
	ModeInput,
} uiMode = ModeSplash;

// Time at which a mode change is allowed
portTickType modeLockedUntil = 0xffffffff;

// for how long a mode is locked
#define MODE_LOCK_MS 250

// Generate Sub modes
static enum {
	GenOff,
	GenTone,
	GenTune1,
	GenTune2,
} generateSubMode = GenOff;

// Util function for sending an event with no data, just a type
void uiSendEvent(UiType type) {
	UiEvent event = {
		.type = type,
	};
	if (!xQueueSendToBack(uiQueue, &event, 0)) {
		uiQueueFullFlag = true;
	}
}

// Timer tick
static void uiTickCallback(xTimerHandle pxTimer) {
	uiSendEvent(UI_TICK);
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
	
	if (uiMode == ModeSplash) {
		// Don't do anything until splash count down
		if (xTaskGetTickCount() >= modeLockedUntil) {
			uiMode = ModeInput;
		}
	} else if (uiMode == ModeInput) {
		ScreenCommand input = {
			.type = SCREEN_INPUT,
			.leftLevel = 30,
			.rightLevel = 70,
			.gain = gain0,
			.fade = 50,
		};
		screenSendCommand(&input);
	} else if (uiMode == ModeGenerate) {
		ScreenCommand generate = {
			.type = SCREEN_GENERATE,
			.off = generateSubMode == GenOff,
			.tone = generateSubMode == GenTone,
			.tune1 = generateSubMode == GenTune1,
			.tune2 = generateSubMode == GenTune2,
			.volume = 196,
			.note = 0x40,  //C4
		};
		screenSendCommand(&generate);
	}
}

// Handle request to turn generation off
static void uiHandleGenerateOff(void) {
	if (uiMode != ModeGenerate) {
		// Should only get this even in generate mode
		return;
	}
	generateSubMode = GenOff;
	audioModeSet(AM_OFF);
	// TODO: turn off tone generator
}

// Handle request to turn tone generation on
static void uiHandleGenerateTone(void) {
	if (uiMode != ModeGenerate) {
		// Should only get this even in generate mode
		return;
	}
	generateSubMode = GenTone;
	audioModeSet(AM_VALUE);
	// TODO: turn on tone generator
}

// Handle request to turn tune1 on
static void uiHandleGenerateTune1(void) {
	if (uiMode != ModeGenerate) {
		// Should only get this even in generate mode
		return;
	}
	generateSubMode = GenTune1;
	audioModeSet(AM_VALUE);
	// TODO: turn on tone generator
}

// Handle request to turn tune2 on
static void uiHandleGenerateTune2(void) {
	if (uiMode != ModeGenerate) {
		// Should only get this even in generate mode
		return;
	}
	generateSubMode = GenTune2;
	audioModeSet(AM_VALUE);
	// TODO: turn on tone generator
}


// Handle request to goto generate page
static void uiHandleGotoGenerate(void) {
	if (xTaskGetTickCount() > modeLockedUntil) {
		uiMode = ModeGenerate;
		uiHandleGenerateOff();
		modeLockedUntil = xTaskGetTickCount() + MS_TO_TICKS(MODE_LOCK_MS);
	}
}

// Handle request to goto input page
static void uiHandleGotoInput(void) {
	if (xTaskGetTickCount() > modeLockedUntil) {
		// TODO(avg): turn off any generation
		uiMode = ModeInput;
		audioModeSet(AM_ADC);
		modeLockedUntil = xTaskGetTickCount() + MS_TO_TICKS(MODE_LOCK_MS);
	}
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
	modeLockedUntil = xTaskGetTickCount() + MS_TO_TICKS(700);
	
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
		} else if (event.type == UI_GOTO_GENERATE) {
			uiHandleGotoGenerate();
		} else if (event.type == UI_GOTO_INPUT) {
			uiHandleGotoInput();
		} else if (event.type == UI_GEN_OFF) {
			uiHandleGenerateOff();
		} else if (event.type == UI_GEN_TONE) {
			uiHandleGenerateTone();
		} else if (event.type == UI_GEN_TUNE1) {
			uiHandleGenerateTune1();
		} else if (event.type == UI_GEN_TUNE2) {
			uiHandleGenerateTune2();
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
	
	xTimerHandle timer = xTimerCreate(USTR("UI_TICK"), MS_TO_TICKS(20), true, NULL, uiTickCallback);
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