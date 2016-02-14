// state.h

#ifndef STATE_H_
#define STATE_H_

// ADC Modes
typedef enum {
	// Initial state. Invalid at other times.
	ADC_NONE,
	ADC_TEST
} ADCMode;

// Global state is modified by passing commands to the StateProcessor
typedef struct {
	// New mode to set or ADC_NONE
	ADCMode adcMode;
	
	// Channel to use in test mode
	int adcTestChannel;
	
} GlobalModCommand;

// The type of the GlobalState structure.
typedef struct {
	// Time of last command
	portTickType lastCommandTicks;
	
	// Current ADC mode
	ADCMode adcMode;
} GlobalState;

// Global state may be read directly from this variable
extern GlobalState GLOBAL_STATE;

// Command to modify the global state
extern void modifyGlobalState(GlobalModCommand *command);

// Start the task for dealing with global state
extern void startGlobalState(void);

#endif /* STATE_H_ */