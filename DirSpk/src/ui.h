// ui.h
//
// UI handling routines and libraries
// 

#ifndef UI_H_
#define UI_H_

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

// Start the UI
void startUi(void);


#endif /* UI_H_ */