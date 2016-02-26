// encoder.h

// Header file for encoder.c
#include <stdbool.h>
#ifndef ENCODER_H_
#define ENCODER_H_

#define NUM_ENCODERS 4

// Encoder pin mask - Pins 12-19 of PIOC
#define ENCODER_PINS 0x000ff000

// Direction encoder last went in
typedef enum {
	ENC_NONE = 0, ENC_CW = 1, ENC_CCW = 2
} EncoderDirection;


// Encoder switch states - kept in 2LSB
typedef uint8_t EncoderSignals;

// Encoder state, Bits 3,2 = last. Bits 1,0 = prev to last
typedef struct {
	// State machine state
	uint8_t machine;
	
	// Last direction and timestamp (for debugging)
	portTickType dirTicks;
	EncoderDirection dir;
} EncoderState;

// Handle PIOC interrupt for interrupt
extern void encoderPIOCHandler(uint32_t, uint32_t);

// Start the encoder subsystem
extern void startEncoders(void);


#endif /* ENCODER_H_ */