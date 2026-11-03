// Engine: decides how each note is tuned.
//
// Told about keys, the pitch wheel, the knobs and the bypass button; tells
// the Synth what to play and the display what to show. Knows nothing about
// the MIDI library, knobs or pins.

#pragma once

#include <stdint.h>

#include "synth.h"
#include "heldNotes.h"

struct EngineSettings {
  int16_t bendRangeCents;       // must match the synth's bend range
  int16_t wheelDeadZone;        // wheel values this close to 0 count as centred
  NotePriority notePriority;    // must match the synth's note priority
};

class Engine {
public:
  Engine(Synth& synth, EngineSettings settings);

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
  EngineSettings settings;

  // Set by setMaqam(); -1 until then.
  int16_t tonic = -1;
  int16_t maqam = -1;

  bool bypass = false;

  // The synth has one bend setting, fed by two sources. We remember both
  // and always send their sum: what you hear = key + maqam offset + wheel.
  int16_t wheelBend = 0;              // where the pitch wheel is (0 = centred)
  int16_t noteBend  = 0;              // maqam offset of the note currently sounding
  uint8_t lastNoteChannel = 1;        // channel of the last note, for bends no MIDI message triggered

  HeldNotes heldNotes;                // keys currently held, in the order pressed
};
