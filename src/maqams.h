// Maqam tables: which keys get bent, and by how many cents.
// Pure music data. No hardware, no MIDI, so it works on any chip.

#pragma once

#include <stdint.h>

// Fixed-width types match the Uno (int16_t = int, int32_t = long), so the
// code behaves the same on any chip and in tests on a computer.

// Note names, indexed by pitch class (C = 0, C# = 1 ... B = 11).
extern const char* noteNames[12];

// A maqam's tuning: a cents offset for each of the 12 keys, counted in
// semitones above the tonic. A half-flat is the natural key bent down
// about 50 cents.
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
