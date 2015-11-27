
/*
* Include header files for all drivers that have been imported from
* Atmel Software Framework (ASF).
*/
#include <asf.h>
#include <string.h>
#include "log.h"
#include "init.h"

// global vars
static uint32_t count40;
static uint32_t ticks = 0;

// Main loop. Executed every tick of TC1.
static void loop(void) {
	if (ticks % 20 == 0) {
		printf("ticks = %ld, count40 = %ld\r\n", ticks, count40);
	}
}

//
// Interrupts
//
// TC0: 40kHz Highest priority ticker
void TC0_Handler(void) {
	// Read ID_TC0 Status to indicate that interrupt has been handled
	tc_get_status(TC0, 0);
	count40++;
	if (count40 >= 40000) {
		count40 = 0;
	}
}


// TC1: 20Hz ticker
void TC1_Handler(void) {
	// Read ID_TC1 Status to indicate that interrupt has been handled
	tc_get_status(TC0, 1);
	ticks++;
	loop();
}


int main(void)
{
	init();
	
	printf("init done\r\n");
	while (1) {
	}
}
