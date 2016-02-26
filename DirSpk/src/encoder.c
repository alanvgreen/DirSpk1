// encoder.c
// 
// Handles rotary encoders on PC12-PC19

#include <asf.h>
#include "encoder.h"
#include "state.h"

// States for state machine.
// See http://www.buxtronix.net/2011/10/rotary-encoders-done-properly.html
//
// The encoder has two binary output signals, making four combinations 00, 01, 10, and 11.
//
// Four sequences cause a direction to be sensed and output from the state machine.
// 00->01->11: ENC_CW (move clockwise)
// 00->10->11: ENC_CCW
// 11->01->00: ENC_CCW
// 11->10->00: ENC_CW
//
// To track these four sequences we need seven states.
//
// 0 = Initial state. Also when previous transition invalid
// 1 = at 00
// 2 = moved from 00 to 01
// 3 = 00->10
// 4 = at 11
// 5 = 11->01
// 6 = 11->10

// State lookup table with row per state, and transitions indexed by EncoderSwitches
// Value encoded in state machine is (direction to output + (next_state << 4))
static int8_t state_table[][4] = {
	{0x10, 0x00, 0x00, 0x40}, // State 0: Lost
	{0x10, 0x20, 0x30, 0x40}, // State 1: at 00
	{0x10, 0x20, 0x00, 0x41}, // State 2: 00->01
	{0x10, 0x00, 0x30, 0x42}, // State 3: 00->10
	{0x10, 0x50, 0x60, 0x40}, // State 4: at 11
	{0x12, 0x50, 0x00, 0x40}, // State 5: 11->01
	{0x11, 0x00, 0x60, 0x40}, // State 6: 11->10
};

// Update a single encoder
// Executes from within an interrupt
static void encoderUpdate(EncoderState *state, EncoderSignals curr) {
	uint8_t instruction = state_table[state->machine][curr];
	state->machine = instruction >> 4;
		
	// Work out what to do next
	EncoderDirection dir = instruction & 3;
	
	// Handle the direction chosen
	if (dir != ENC_NONE) {
		state->dirTicks = xTaskGetTickCount();
		state->dir = dir;
	}
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
		// GLOBAL_STATE.encoders[i];
	}
}
