// Engine: decides how each note is tuned, and tells the Synth and Display.
// It has no hardware dependencies, so it also runs in the host tests.

#pragma once

#include <stdint.h>

#include "synth.h"
#include "display.h"
#include "heldNotes.h"

// See settings.h.
struct EngineSettings {
  int16_t bendRangeCents;
  int16_t wheelDeadZone;
  NotePriority notePriority;
};

class Engine {
public:
  Engine(Synth& synth, Display& display, EngineSettings settings);

  void noteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  void noteOff(uint8_t channel, uint8_t note, uint8_t velocity);
  void wheel(uint8_t channel, int16_t bend);
  void setMaqam(int16_t tonic, int16_t maqam);
  void toggleBypass();
  bool bypassed() const;

private:
  int16_t sendCombinedBend(uint8_t channel);
  void tuneSoundingNote(uint8_t channel);
  void retuneHeldNote();

  Synth& synth;
  Display& display;
  EngineSettings settings;

  // Start at 0 rather than a "not set" value, so maqams[maqam] is always valid.
  int16_t tonic = 0;
  int16_t maqam = 0;
  bool bypass = false;

  // The synth has one bend per channel, and two things want to set it: the
  // wheel and the maqam offset. Every bend sent is their sum.
  int16_t wheelBend = 0;
  int16_t noteBend  = 0;

  // Retuning after a knob turn or bypass isn't triggered by a MIDI message,
  // so it uses the channel of the last note.
  uint8_t lastNoteChannel = 1;

  HeldNotes heldNotes;
};
