// The keys currently held down, in the order they were pressed.
// A mono synth remembers held keys and falls back to one when the sounding
// key is released; we keep the same list so we know which note to tune.
// Pure logic. No hardware, no MIDI, so it works on any chip.

#pragma once

#include <stdint.h>

// Fixed-width types match the Uno (int16_t = int, int32_t = long), so the
// code behaves the same on any chip and in tests on a computer.

// How many held keys we remember. Ten fingers; if more arrive, the oldest is dropped.
const int16_t MAX_HELD_NOTES = 10;

// Which held key a mono synth plays. Must match the synth's own setting.
enum class NotePriority {
  Last,   // the most recently pressed key wins
  Low,    // the lowest held key wins
  High,   // the highest held key wins
};

class HeldNotes {
public:
  // Put a key on the end of the list (newest). A key that's already
  // listed moves to the end rather than being listed twice.
  void add(uint8_t note);

  // Take a key off the list, if it's there.
  void remove(uint8_t note);

  // Is any key held?
  bool any() const;

  // Which held key the synth is sounding, by its note priority.
  // Only call when any() is true.
  uint8_t sounding(NotePriority priority) const;

private:
  uint8_t notes[MAX_HELD_NOTES];   // oldest first; only the first `count` are in use
  int16_t count = 0;
};
