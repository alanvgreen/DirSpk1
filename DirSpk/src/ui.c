// ui.c
//
// The UI handling task and code

#include "decls.h"

// UiQueue for events from encoders, touch, etc. Contains 
xQueueHandle uiQueue;

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
static portTickType modeLockedUntil = 0xffffffff;

// for how long a mode is locked
#define MODE_LOCK_MS 250

// Generate Sub modes
static enum {
	GenOff,
	GenTone,
	GenTune1,
	GenTune2,
} generateSubMode = GenOff;

// For generated tones
// TODO: get these values from EEPROM?
static uint8_t volume = 220; 
static uint8_t note = 0x40; 

// For input mode
// TODO: get these values from EEPROM
static uint8_t gain;
static uint8_t fade;

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
	
	
	// TODO: incorporate gain and fade
	/*
	uint16_t vol = gain0;
	// Set reg0
	spiSendReceive((0 << 12) + vol);
	// Set reg1
	spiSendReceive((1 << 12) + vol);
	*/
}


// Handled encoder moves
static void uiHandleEncoderEvent(EncoderMove *p) {
	// TODO: refit pots and re-instrument
	if (uiMode == ModeGenerate) {
		if (p->num == 0) {
			if (p->dir == ENC_CW) {
				note = noteIncrement(note);
			} else {
				note = noteDecrement(note);
			}
		}
		if (p->num == 1) {
			if (volume > 0 && p->dir == ENC_CCW) {
				volume--;
			} else if ((volume < 255) & (p->dir == ENC_CW)) {
				volume++;
			}
		}
	}
	
	// TODO: input mode
	
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
			audioModeSet(AM_ADC);
		}
	} else if (uiMode == ModeInput) {
		ScreenCommand input = {
			.type = SCREEN_INPUT,
			.leftLevel = 30,
			.rightLevel = 70,
			.gain = 23,
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
			.volume = volume,
			.note = note
		};
		screenSendCommand(&generate);
		
		// TODO: either set mode and frequency here 
		// OR change it with changing input.
		if (generateSubMode == GenTone) {
			audioModeSet(AM_HZ);
			audioFrequencySet(noteToFrequency(note));
		}
		audioVolume = volume;
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
}

// Handle request to turn tune1 on
static void uiHandleGenerateTune1(void) {
	if (uiMode != ModeGenerate) {
		// Should only get this even in generate mode
		return;
	}
	generateSubMode = GenTune1;
	audioModeSet(AM_HZ);
	// Middle C - 261Hz
	audioFrequencySet(noteToFrequency(0x40));
}

// Handle request to turn tune2 on
static void uiHandleGenerateTune2(void) {
	if (uiMode != ModeGenerate) {
		// Should only get this even in generate mode
		return;
	}
	generateSubMode = GenTune2;
	audioModeSet(AM_HZ);
	// 4 octaves above middle c - 4.186kHz
	audioFrequencySet(noteToFrequency(0x80)); 
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
	// TODO: figure gain, fade
	/*
	// Read r0
	gain0 = spiSendReceive(0x0c00) & 0x1ff;

	// Read r1
	gain1 = spiSendReceive(0x1c00) & 0x1ff;	
	*/
}

// The ui coordinator task.
static void uiTask(void *pvParameters) {
	// TODO: verify 220, and change scaling in audio.c so 255 is the new max
	audioVolume = 220; // Maximum usable volume, as it turns out
	
	// Initialize the UI global state
	spiWithMutex(getVolume);
	
	// Show startup screen
	ScreenCommand init = {.type = SCREEN_INIT};
	screenSendCommand(&init);
	ScreenCommand startup = {.type = SCREEN_STARTUP};
	screenSendCommand(&startup);
	vTaskDelay(MS_TO_TICKS(700));

	// Prevent change of screen mode while startup is displayed
	modeLockedUntil = xTaskGetTickCount() + MS_TO_TICKS(700);
	
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