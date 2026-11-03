// Synth: what the engine needs from the synth it's playing.
//
// An interface: it lists the operations but doesn't say how they're done.
// The real version (MidiSynth in maqamatron.ino) sends MIDI; a test version
// could just record the calls. The engine only ever sees this, so it never
// depends on the MIDI library.

#pragma once

#include <stdint.h>

class Synth {
public:
  virtual void noteOn(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
  virtual void noteOff(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
  virtual void bend(uint8_t channel, int16_t bend) = 0;       // -8192..8191, 0 = centred

  // Bypass: forward every incoming message to the synth untouched.
  virtual void passThrough(bool on) = 0;
};
