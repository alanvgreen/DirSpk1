/*
 * util.h
 *
 * General utility functions
 */ 

#include <asf.h>
#include "init.h"

#ifndef UTIL_H_
#define UTIL_H_

// Assert true or blink
#define ASSERT_BLINK(c, l, s) { if (!c) fatalBlink(l, s); }

// Blink out a pattern in death.
extern void fatalBlink(short longBlinks, short shortBlinks);

// For FreeRTOS object names and other strings
#define USTR(n) ((const int8_t *const) n)

// convert milliseconds to ticks
#define MS_TO_TICKS(t) ((t) / portTICK_RATE_MS)


#endif /* UTIL_H_ */