// init.c
//
// General device initialization

#include <asf.h>
#include "init.h"

static void initGpio(void) {
	ioport_init();
	
	// PIOB
	ioport_set_pin_level(LED0_GPIO, false);
	ioport_set_pin_dir(LED0_GPIO, IOPORT_DIR_OUTPUT);
}

void init(void) {
	sysclk_init();
	NVIC_SetPriorityGrouping(0);
	board_init();
	
	initGpio();
}