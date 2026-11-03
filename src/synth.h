// The synth the engine plays. MidiSynth in maqamatron.ino is the real one;
// the tests use a fake that records calls.

#pragma once

#include <stdint.h>

class Synth {
public:
  virtual void noteOn(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
  virtual void noteOff(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
  virtual void bend(uint8_t channel, int16_t bend) = 0;       // -8192..8191

  // Bypass: forward all incoming MIDI unchanged.
  virtual void passThrough(bool on) = 0;
};
