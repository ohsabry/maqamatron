#include "engine.h"

#include "maqams.h"
#include "bend.h"

Engine::Engine(Synth& synth, Display& display, EngineSettings settings)
  : synth(synth), display(display), settings(settings) {}

bool Engine::bypassed() const {
  return bypass;
}

// noteOn, noteOff and wheel keep their state up to date even in bypass,
// so leaving bypass tunes whatever is held at that moment.

void Engine::noteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  heldNotes.add(note);
  lastNoteChannel = channel;
  if (bypass) {
    return;
  }

  // Bend first, so the note doesn't start out of tune.
  tuneSoundingNote(channel);
  synth.noteOn(channel, note, velocity);
}

void Engine::noteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  heldNotes.remove(note);
  if (bypass) {
    return;
  }

  synth.noteOff(channel, note, velocity);

  // A mono synth falls back to another held key, which needs its own bend.
  if (heldNotes.any()) {
    tuneSoundingNote(channel);
  }
}

void Engine::wheel(uint8_t channel, int16_t bend) {
  wheelBend = applyWheelDeadZone(bend, settings.wheelDeadZone);
  if (bypass) {
    return;
  }
  sendCombinedBend(channel);
}

void Engine::setMaqam(int16_t newTonic, int16_t newMaqam) {
  tonic = newTonic;
  maqam = newMaqam;
  display.showMaqam(maqams[maqam].name, tonic, bypass);
  retuneHeldNote();
}

void Engine::toggleBypass() {
  bypass = !bypass;

  if (bypass) {
    // Thru won't send a bend, so the synth would stay bent by the last
    // maqam offset. Drop it now, keeping the wheel.
    noteBend = 0;
    sendCombinedBend(lastNoteChannel);
    synth.passThrough(true);
  } else {
    synth.passThrough(false);
    retuneHeldNote();
  }

  display.showMaqam(maqams[maqam].name, tonic, bypass);
}

void Engine::setSettings(EngineSettings newSettings) {
  settings = newSettings;
}

int16_t Engine::sendCombinedBend(uint8_t channel) {
  int16_t total = combineBend(wheelBend, noteBend);
  synth.bend(channel, total);
  return total;
}

void Engine::tuneSoundingNote(uint8_t channel) {
  uint8_t note  = heldNotes.sounding(settings.notePriority);
  int16_t cents = maqamCentsForNote(note, tonic, maqam);
  noteBend      = centsToBend(cents, settings.bendRangeCents);
  int16_t bend  = sendCombinedBend(channel);

  display.showNote(note, cents, bend);
}

void Engine::retuneHeldNote() {
  if (heldNotes.any() && !bypass) {
    tuneSoundingNote(lastNoteChannel);
  }
}
