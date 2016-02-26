//
// init.h
//
// Initialization and I/O Configuration
//
// Pin usage:
//
// * LED0_GPIO = B27 = Due D13 

#include "freertos_peripheral_control.h"

#ifndef INIT_H_
#define INIT_H_

// UART interface for use by CLI
extern freertos_uart_if freeRTOSUART;

// General start-of-execution initialization
extern void init(void);

#endif /* INIT_H_ */