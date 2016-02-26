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
	ENC_NONE, ENC_CW, ENC_CCW
} EncoderDirection;

// Encoder switch states - kept in 2LSB
typedef uint8_t EncoderSwitches;

// Encoder state, Bits 3,2 = last. Bits 1,0 = prev to last
typedef struct {
	// Last state
	EncoderSwitches last;
	// Previous state to last
	EncoderSwitches prev;
	
	// Last direction and timestamp (for debugging)
	portTickType dirTicks;
	EncoderDirection dir;
} EncoderState;

// Handle PIOC interrupt for interrupt
extern void encoderPIOCHandler(uint32_t, uint32_t);

// Start the encoder subsystem
extern void startEncoders(void);


#endif /* ENCODER_H_ */