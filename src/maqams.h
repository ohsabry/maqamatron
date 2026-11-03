// Maqam tuning tables.

#pragma once

#include <stdint.h>

extern const char* noteNames[12];   // by pitch class, C = 0

// Cents offset for each of the 12 semitones above the tonic.
struct Maqam {
  const char* name;
  int16_t cents[12];
};

extern const Maqam maqams[];        // in knob order
extern const int16_t maqamCount;

// tonic is a pitch class, 0-11.
int16_t maqamCentsForNote(uint8_t note, int16_t tonic, int16_t maqamIndex);
