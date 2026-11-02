// Maqam tables: which keys get bent, and by how many cents.
// Pure music data. No hardware, no MIDI, so it works on any chip.

#pragma once

#include <stdint.h>

// Fixed-size types mirror the Arduino Uno, so the code means the same on
// any chip (and in tests on a computer): int16_t is the Uno's int,
// int32_t is the Uno's long, uint8_t is a byte.

// Note names, indexed by pitch class (C = 0, C# = 1 ... B = 11).
extern const char* noteNames[12];

// Cents offset for each key, counted in semitones above the tonic.
struct Maqam {
  const char* name;
  int16_t cents[12];
};

// All maqamat, in maqam knob order. Defined in maqams.cpp.
extern const Maqam maqams[];
extern const int16_t maqamCount;

// The offset in cents for a key, in the given maqam on the given tonic.
// note: MIDI note number, tonic: pitch class 0-11, maqamIndex: index into maqams.
int16_t maqamCentsForNote(uint8_t note, int16_t tonic, int16_t maqamIndex);
