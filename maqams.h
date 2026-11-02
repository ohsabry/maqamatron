// Maqam tables: which keys get bent, and by how many cents.
// Pure music data. No hardware, no MIDI, so it works on any chip.

#pragma once

#include <stdint.h>   // uint8_t

// Note names, indexed by pitch class (C = 0, C# = 1 ... B = 11).
extern const char* noteNames[12];

// Cents offset for each key, counted in semitones above the tonic.
struct Maqam {
  const char* name;
  int cents[12];
};

// All maqamat, in maqam knob order. Defined in maqams.cpp.
extern const Maqam maqams[];
extern const int maqamCount;

// The offset in cents for a key, in the given maqam on the given tonic.
// note: MIDI note number, tonic: pitch class 0-11, maqamIndex: index into maqams.
int maqamCentsForNote(uint8_t note, int tonic, int maqamIndex);
