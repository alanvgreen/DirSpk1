// state.h

#ifndef STATE_H_
#define STATE_H_

#include <stdbool.h>
#include "encoder.h"

// The type of the GlobalState structure.
//
// Contains all state that is shared between components, particularly if 
// displayable in UI or in CLI.
// 
// Non-concurrent members are commented with an owner.
// Only the owner updates the value.
typedef struct {
	// UiQueue for events from encoders, touch, etc. 
	xQueueHandle uiQueue;
	
	// Volume for channels zero and 1. Owner: UI task.
	uint16_t volume0, volume1;
	
	// Whether UIQueue has ever been full. Set only. Owner: any sender to uiQueue.
	bool uiQueueFullFlag;
	
	// Whether PWM is enabled. Set by calling xxxx
	// bool pwmEnable;
	// Whether DAC is enabled. Set by calling xxxx
	// bool dacEnable;
	// How Audio signals are generated. Set by calling xxx
	// AudioInMode audioInMode;
	
	// State of the encoders. Owner: Encoder interrupt.
	EncoderState encoders[NUM_ENCODERS];
	
	// Debug queue. All moves go in here, although queue may overflow
	xQueueHandle encoderDebugQueue;
	
	// Mutex for SPI is being used.
	xSemaphoreHandle spiMutex;
	
} GlobalState;

// Global state may be read directly from this variable
extern GlobalState GLOBAL_STATE;

#endif /* STATE_H_ */