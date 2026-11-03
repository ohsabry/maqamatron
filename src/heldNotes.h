// The keys held down, oldest first. A mono synth falls back to a held key
// when the sounding one is released; this mirrors its list, so we know
// which note to tune.

#pragma once

#include <stdint.h>

const int16_t MAX_HELD_NOTES = 10;   // when full, the oldest is dropped

// Which held key a mono synth sounds.
enum class NotePriority {
  Last,
  Low,
  High,
};

class HeldNotes {
public:
  // A key already held moves to the end instead of being listed twice.
  void add(uint8_t note);
  void remove(uint8_t note);
  bool any() const;

  // Only valid when any() is true.
  uint8_t sounding(NotePriority priority) const;

private:
  uint8_t notes[MAX_HELD_NOTES];
  int16_t count = 0;
};
