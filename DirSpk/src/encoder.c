// encoder.c
// 
// Handles rotary encoders on PC12-PC19

#include <asf.h>
#include "encoder.h"
#include "state.h"


// Update a single encoder
// Executes from within an interrupt
static void encoderUpdate(EncoderState *state, EncoderSwitches curr) {
	// If curr same as prev, ignore the update
	if (curr == state->last) {
		return;
	}
	
	// Work out what to do next
	EncoderDirection dir = ENC_NONE;
	switch ((curr << 4) + (state->last << 2) + state->prev) {
		case 0x03: dir = ENC_CCW; break;
		case 0x0b: dir = ENC_CW; break;
		case 0x34: dir = ENC_CW; break;
		case 0x38: dir = ENC_CCW; break;
		// Unlisted states require no special processing;
		default: break;
	}
	
	// Handle the direction chosen
	if (dir != ENC_NONE) {
		state->dirTicks = xTaskGetTickCount();
		state->dir = dir;
	}

	// time moves forward
	state->prev = state->last;
	state->last = curr;
}

// Handle PIOC interrupt
void encoderPIOCHandler(uint32_t unused_id, uint32_t unused_mask) {
	// Get new values of pins
	uint32_t curr = PIOC->PIO_PDSR >> 12;
	
	for (int i = 0; i < NUM_ENCODERS; i++) {
		encoderUpdate(GLOBAL_STATE.encoders + i, curr & 3);
		curr >>= 2;
	}
}

// Start the encoder subsystem
void startEncoders(void) {
	for (int i = 0; i < NUM_ENCODERS; i++) {
		GLOBAL_STATE.encoders[i]
	}
}
