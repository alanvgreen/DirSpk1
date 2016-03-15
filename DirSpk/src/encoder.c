// encoder.c
//
// Handles rotary encoders on PC12-PC19
//
// Runs an interrupt on change in encoder GPIO pin inputs. When the interrupt logic determines
// that there has been a direction change, items are placed on RTOS queues for interpretation
// by the UI and CLI tasks.

#include "decls.h"

// State of the encoders. Owner: Encoder interrupt.
// Exposed for debug through CLI.
EncoderState encoderStates[NUM_ENCODERS];

// For debugging. Holds EncoderMove objects.
// All moves go in here, although queue may overflow.
xQueueHandle encoderDebugQueue;

// An encoder is a device that indicates movement clockwise or counter-clockwise. By itself, it
// has no absolute position. Encoders have two switches each providing a binary output signal,
// making four combinations - 00, 01, 10, and 11.
//
// To properly interpret these signals requires knowing not just the current value of the two
// output switches but also the previous values. These values are tracked using a small state
// machine. I thought about this for a long time and ended up completely agreeing with the principles
// put forth by Ben Buxton at the link below. I then borrowed implementation concepts from his
// code as well.
//
// See http://www.buxtronix.net/2011/10/rotary-encoders-done-properly.html
//
// For our purposes, there are two kinds of encoders:
//   A) Those designed to signal a move at both position 00 and 11, and
//   B) Those designed to only indicate a move at position 00.
//
// For type A converters, there are four sequences of encoder position that are relevant to indicating
// a movement:
// 00->01->11: ENC_CW (move clockwise)
// 00->10->11: ENC_CCW
// 11->01->00: ENC_CCW
// 11->10->00: ENC_CW
//
// Simply tracking (say) 01->11 is insufficient to indicate a clockwise movement as the sequence 11->01->11
// does not indicate a movement - it could be the result of the encoder switch resting near a transition.
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

// State lookup table A with row per state, and transitions indexed by EncoderSwitches
// Value encoded in state machine is (direction to output + (next_state << 4))
static uint8_t stateTableA[][4] = {
	{0x10, 0x00, 0x00, 0x40}, // State 0: Lost
	{0x10, 0x20, 0x30, 0x40}, // State 1: at 00
	{0x10, 0x20, 0x00, 0x41}, // State 2: 00->01
	{0x10, 0x00, 0x30, 0x42}, // State 3: 00->10
	{0x10, 0x50, 0x60, 0x40}, // State 4: at 11
	{0x12, 0x50, 0x00, 0x40}, // State 5: 11->01
	{0x11, 0x00, 0x60, 0x40}, // State 6: 11->10
};

//
// Similarly, type B encoders have two sequences that indicate state change:
// 00->01->11->10->00: ENC_CW
// 00->10->11->01->00: ENC_CCW
//
// It is allowed to step backwards through intermediates states so that:
// 00->01->11->01->11->10->11->10->00 also indicates ENC_CW
//
// We use these states:
// 0: Lost. Looking for 00
// 1: At 00
// 2: 00->01
// 3: 00->01->11
// 4: 00->01->11->10
// 5: 00->10
// 6: 00->10->11
// 7: 00->10->11->01

// State lookup table B with row per state, and transitions indexed by EncoderSwitches
// Value encoded in state machine is (direction to output + (next_state << 4))
static uint8_t stateTableB[][4] = {
	{0x10, 0x00, 0x00, 0x00}, // State 0: Lost
	{0x10, 0x20, 0x50, 0x00}, // State 1: at 00
	{0x10, 0x20, 0x00, 0x30}, // State 2: 00->01
	{0x10, 0x20, 0x40, 0x30}, // State 3: 00->01->11
	{0x11, 0x00, 0x40, 0x30}, // State 4: 00->01->11->10
	{0x10, 0x00, 0x50, 0x60}, // State 5: 00->10
	{0x10, 0x70, 0x50, 0x60}, // State 6: 00->10->11
	{0x12, 0x70, 0x00, 0x60}, // State 7: 00->10->11->01
};

// Update a single encoder
// Executes from within an interrupt
static void encoderUpdate(int num, EncoderSignals signals) {
	EncoderState *encoder = encoderStates + num;
	
	// Look up instruction in this encoder's state table
	uint8_t instruction = encoder->table[encoder->state][signals];
	encoder->state = instruction >> 4;
	
	// Work out what to do next
	EncoderDirection dir = instruction & 3;
	
	// Update debug info if input signals differ from last signals
	portTickType now = xTaskGetTickCount();
	if (encoder->previousSignals[0] != signals) {
		for (int i = 9; i > 0; i--) {
			encoder->previousStates[i] = encoder->previousStates[i-1];
			encoder->previousSignals[i] = encoder->previousSignals[i-1];
			encoder->previousDirections[i] = encoder->previousDirections[i-1];
			encoder->timeSince[i] = encoder->timeSince[i-1];
		}
		encoder->previousStates[0] = encoder->state;
		encoder->previousSignals[0] = signals;
		encoder->previousDirections[0] = dir;
		encoder->timeSince[0] = now - encoder->lastChange;
		encoder->lastChange = now;
	}
	
	// No direction sensed - get out now
	if (dir == ENC_NONE) {
		return;
	}
	
	// Record a UI Event
	UiEvent event = {
		.type = UI_ENCODER,
		.encMove = {
			.num = num,
			.when = now,
			.dir = dir,
		}
	};
	
	portBASE_TYPE taskWoken = false;
	if (GLOBAL_STATE.uiQueue) {
		if (!xQueueSendToBackFromISR(GLOBAL_STATE.uiQueue, &event, &taskWoken)) {
			GLOBAL_STATE.uiQueueFullFlag = true;
		}
	}
	if (encoderDebugQueue) {
		xQueueSendToBackFromISR(encoderDebugQueue, &event.encMove, &taskWoken);
	}
	if (taskWoken) {
		vPortYieldFromISR();
	}
}

// Handle PIOC interrupt
void encoderPIOCHandler(uint32_t unused_id, uint32_t unused_mask) {
	// Get new values of pins from bits 19-12
	uint32_t curr = PIOC->PIO_PDSR >> 12;
	
	//encoderUpdate(0, curr&3);
	for (int i = 0; i < NUM_ENCODERS; i++) {
		encoderUpdate(i, curr & 3);
		curr >>= 2;
	}
}

// Start the encoder subsystem
void startEncoders(void) {
	// Create the debug queue
	encoderDebugQueue = xQueueCreate(5, sizeof(EncoderMove));
	
	// Set up the state table for each encoder
	encoderStates[0].table = stateTableB;
	encoderStates[1].table = stateTableB;
	encoderStates[2].table = stateTableA;
	encoderStates[3].table = stateTableA;
}


static char const *SEP = ", ";

// For debugging, print the encoder state
void encoderStateSprint(char *buf, size_t buflen, EncoderState *encoder) {
	char *p = buf;
	int remaining = buflen;
	for (int i = 0; i < 10; i++) {
		int r = snprintf(p, remaining, "%s[%1hhx %1hhd %1d %3lu]", 
		    i > 0 ? SEP : "", 
			encoder->previousStates[i],
			encoder->previousSignals[i],
			encoder->previousDirections[i],
			encoder->timeSince[i]);
		if (r <= 0 || r > remaining) {
			// Some kind of output or truncation
			return ;
		}
		// Point to new write location, decrement remaining buffer size
		// in unison.
		p += r;
		remaining -= r;
	}
}