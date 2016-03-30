#!/usr/bin/python
#
# Builds C code that embeds tables of notes that make up tunes.
#
# This output is a complete, though ugly, source file.
#
# Built with Python2.7
#

ODE_TO_JOY = [
    ("B4", 0.25), ("B4", 0.25), ("C5", 0.25), ("D5", 0.25),
    ("D5", 0.25), ("C5", 0.25), ("B4", 0.25), ("A4", 0.25),
    ("G4", 0.25), ("G4", 0.25), ("A4", 0.25), ("B4", 0.25),
    ("B4", 0.375), ("A4", 0.125), ("A4", 0.50),

    ("B4", 0.25), ("B4", 0.25), ("C5", 0.25), ("D5", 0.25),
    ("D5", 0.25), ("C5", 0.25), ("B4", 0.25), ("A4", 0.25),
    ("G4", 0.25), ("G4", 0.25), ("A4", 0.25), ("B4", 0.25),
    ("A4", 0.375), ("G4", 0.125), ("G4", 0.50),

    ("A4", 0.50), ("B4", 0.25), ("G4", 0.25),
    ("A4", 0.25), ("B4", 0.125), ("C5", 0.125), ("B4", 0.25), ("G4", 0.25),
    ("A4", 0.25), ("A4", 0.25), ("B4", 0.25), ("A4", 0.25),
    ("G4", 0.375), ("A4", 0.125), ("D5", 0.50),

    ("B4", 0.25), ("B4", 0.25), ("C5", 0.25), ("D5", 0.25),
    ("D5", 0.25), ("C5", 0.25), ("B4", 0.25), ("A4", 0.25),
    ("G4", 0.25), ("G4", 0.25), ("A4", 0.25), ("B4", 0.25),
    ("A4", 0.375), ("G4", 0.125), ("G4", 0.50),
]

# beats per minute and MS per whole note (4 beats)
BPM = 96
MS_PER_WHOLE = 60.0 / BPM * 4 * 1000
INTER_NOTE_PAUSE_MS = 16
NOTES = {
    'C': 0,
    'C#': 1,
    'Db': 1,
    'D': 2,
    'D#': 3,
    'Eb': 3,
    'E': 4,
    'F': 5,
    'F#': 6,
    'Gb': 6,
    'G': 7,
    'G#': 8,
    'Ab': 8,
    'A': 9,
    'A#': 10,
    'Bb': 10,
    'B': 11
}

def encode_note(note_str):
    # Octave goes into high nybble, note into low nybble
    return "0x%c%1x" % (note_str[1], NOTES[note_str[0]])

def encode_end(time):
    # Integer division
    return time / 16

def print_data(var_name, data_in, tempo):
    end_time = 0
    print "TuneData %s[] = {" % var_name
    for (note_str, duration) in data_in:
        end_time += int(duration * MS_PER_WHOLE) - INTER_NOTE_PAUSE_MS
        print "  {%s, 0x%04x}," % (encode_note(note_str), encode_end(end_time))
        end_time += INTER_NOTE_PAUSE_MS
        print "  {0x00, 0x%04x}," % encode_end(end_time)
    print "  {0x00, 0xffff},"
    print "};"

print """// Generated tunes file.
//
// The code to generate this file is in make_tables.py. Do not edit by hand -
// change and re-run the python instead.

#include "decls.h"
"""

print "// Ode To Joy"
print_data("tune1", ODE_TO_JOY, 500);
