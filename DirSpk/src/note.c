// note.c utilities for handling musical notes

#include "decls.h"

// Notes are composed of two nibbles
// The first gives the octave number and the second gives the note number.
// Notes range from 0 - 7, where:
// 0: c
// 1: c#
// 2: d
// 3: d#
// 4: e
// 5: f
// 6: f#
// 7: g
// 8: g#
// 9: a
// a: a#
// b: b
//
// 40 is middle C (C4), and 49 is middle A (A4) = 440Hz
//

// Table of frequencies for the 10th octave, generated with this python snippet
// >>> x = math.pow(2.0, (1./12))
// >>> for i in range(0, 12):
// ...     print 440 * (2**6) * pow(x, (i-9))
// The tenth octave is well beyond human hearing, and mostly beyond the speaker
// but that's fine.
static uint32_t freqTable[] = {
    16744,
	17740,
	18795,
	19912,
	21096,
	22350,
	23679,
	25087,
	26580,
	28160,
	29834,
	31609
};

// From a numbered note, derive a frequency. Returns 0 on error
extern uint32_t noteToFrequency(uint8_t note) {
	uint8_t i = note & 0xf;
	if (i >= 12) {
		return 0;
	}
	uint16_t o = (note & 0xf0) >> 4;
	if (o > 10) {
		return 0;
	}
	return freqTable[i] >> (10 - o);
}


// Table of indexes to notes
static char const *nameTable[] = {
	"C", "C#", "D", "D#", "E", "F",
	"F#", "G", "G#", "A", "A#", "B",
};

// From a numbered note return a name. p must be at least 4 chars long 
// (3 chars + \0)
extern void noteToName(uint8_t note, char *p) {
	uint8_t i = note & 0xf;
	uint16_t o = (note & 0xf0) >> 4;
	snprintf(p, 4, "%s%1u", i < 12 ? nameTable[i] : "?", o < 10 ? o : 0);
}