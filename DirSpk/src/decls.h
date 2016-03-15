// Common declarations file for DirSpk1

#ifndef DECLS_H_
#define DECLS_H_

//
// Library files
//
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

//
// Auto-generated ASF header - includes all configured ASF components
//
#include <asf.h>

//////////////////////////////////
// Project specific declarations
//////////////////////////////////

//
// Useful macros
//

// For FreeRTOS object names and other strings
#define USTR(n) ((const int8_t *const) (n))

// convert milliseconds to ticks
#define MS_TO_TICKS(t) ((t) / portTICK_RATE_MS)

//
// PWM Generation / Audio
//

// Ultrasonic PWM period in microseconds = 84000000 / 40000 cycles
#define US_PERIOD 2100

// The various modes that the audio can be in
typedef enum {
	// No Audio - turn off PWM
	AM_OFF,
	
	// Via ADC channels
	AM_ADC,
	
	// Given Value to output
	AM_FREQ	
} AudioMode;

// The current mode
extern AudioMode audioMode;

// The value to output - range from 1 to US_PERIOD - 2, inclusive
extern uint16_t audioValue;


//
// Encoders
//

#define NUM_ENCODERS 4

// Encoder pin mask - Pins 12-19 of PIOC
#define ENCODER_PINS 0x000ff000

// Direction sensed by Encoder - none, clockwise, counter-clockwise
typedef enum {
	ENC_NONE = 0, ENC_CW = 1, ENC_CCW = 2
} EncoderDirection;

// Encoders signal through two switches. Their states are kept in 
// this EncoderSignals type - in the two LSB.
typedef uint8_t EncoderSignals;

// Everything for a single encoder 
typedef struct {
	// The state machine table to use - different encoders require
	// different tables to run correctly.
	uint8_t (*table)[4];
	
	// Current state number
	uint8_t state;
	
	// Debugging info - 0 is most recent, 9 is least recent
	// Only contains information if signals differ from previously received
	portTickType lastChange;
	uint8_t previousStates[10];
	EncoderSignals previousSignals[10]; 
	EncoderDirection previousDirections[10];
	uint32_t timeSince[10];
} EncoderState;

// For debugging, print the encoder state
extern void encoderStateSprint(char *buf, size_t buflen, EncoderState *encoder);

// Information about a sensed direction for a particular encoder.
// This struct will be sent through queues to tasks.
typedef struct {
	// Which encoder this is
	int num;
	
	// A timestamp
	portTickType when;
	
	// When this happened
	EncoderDirection dir;
} EncoderMove;

// Handle PIOC interrupt for interrupt
extern void encoderPIOCHandler(uint32_t, uint32_t);

// Start the encoder subsystem
extern void startEncoders(void);

// State of the encoders. Owner: Encoder interrupt.
// Exposed for debug through CLI.
extern EncoderState encoderStates[NUM_ENCODERS];

// For debugging. Holds EncoderMove objects.
// All moves go in here, although queue may overflow.
extern xQueueHandle encoderDebugQueue;

//
// SPI control
//

// Start the SPI subsystem
void startSpi(void);

// Mutex for SPI is being used.
// Tasks calling spiSendReceive must hold this mutex.
extern xSemaphoreHandle spiMutex;

// Send a datum, receive a datum.
// Channel number must be encoded in bits 19-16
// Acquire semaphore before using
uint32_t spiSendReceive(uint32_t datum);

// Utility function: executes fn while holding the SPI mutex
void spiWithMutex(void (*fn)(void));


//
// Utility funcs
//

// Assert true or fatal blink
#define ASSERT_BLINK(c, l, s) { if (!(c)) fatalBlink((l), (s)); }

// Blink out a pattern to signal error.
extern void errorBlink(short longBlinks, short shortBlinks);

// Blink out a pattern for ever.
extern void fatalBlink(short longBlinks, short shortBlinks);


//
// Initialization of MCU & peripherals
//

// Initialize everything
extern void init(void);

//
// Command line interface
//

// UART interface for use by CLI
extern freertos_uart_if freeRTOSUART;

// Start the command line interface.
extern void startCli(void);

//
// UI - knobs, screen, buttons, etc
//

// Type of queue item
typedef enum {
	UI_ENCODER
} UiType;

// The Queue item
typedef struct {
	UiType type;
	union {
		EncoderMove encMove;
	};
} UiEvent;

// UiQueue for events from encoders, touch, etc. Contains UiEvents
extern xQueueHandle uiQueue;
	
// Gain for channels zero and 1. Written by: UI task.
// This is a convenient reflection of the last set values.
extern volatile uint16_t gain0, gain1;
	
// Whether UIQueue has ever been full. Set only. Written by: any sender to uiQueue.
extern volatile bool uiQueueFullFlag;

// Start the UI task
void startUi(void);

#endif /* DECLS_H_ */