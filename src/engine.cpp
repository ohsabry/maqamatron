#include "engine.h"

#include "maqams.h"
#include "bend.h"
#include "display.h"

Engine::Engine(Synth& synth, EngineSettings settings)
  : synth(synth), settings(settings) {}

bool Engine::bypassed() const {
  return bypass;
}


// ===== Keys and wheel =====

// These keep heldNotes, wheelBend and lastNoteChannel up to date even in
// bypass, so they're correct when bypass is switched off.

void Engine::noteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  heldNotes.add(note);
  lastNoteChannel = channel;
  if (bypass) {
    return;                                 // thru already forwarded it
  }

  tuneSoundingNote(channel);                // bend first (offset + wheel)...
  synth.noteOn(channel, note, velocity);    // ...then the note
}

void Engine::noteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  heldNotes.remove(note);
  if (bypass) {
    return;                                 // thru already forwarded it
  }

  synth.noteOff(channel, note, velocity);

  // If keys are still held, the synth falls back to one of them:
  // send its bend. (If an older key was released, the sounding key
  // hasn't changed and this just re-sends the same bend.)
  if (heldNotes.any()) {
    tuneSoundingNote(channel);
  }
}

// No display here: a moving wheel sends many messages a second, and
// drawing for each one would slow MIDI down.
void Engine::wheel(uint8_t channel, int16_t bend) {
  wheelBend = applyWheelDeadZone(bend, settings.wheelDeadZone);

  if (bypass) {
    return;                                 // thru already forwarded it
  }
  sendCombinedBend(channel);
}


// ===== Knobs and bypass =====

void Engine::setMaqam(int16_t newTonic, int16_t newMaqam) {
  tonic = newTonic;
  maqam = newMaqam;
  displaySetMaqam(maqams[maqam].name, tonic, bypass);
  retuneHeldNote();                         // a held key takes the new tuning now
}

// True bypass: the synth passes every incoming message through untouched,
// and noteOn/noteOff/wheel stay out of the way.
void Engine::toggleBypass() {
  bypass = !bypass;

  if (bypass) {
    // The synth may still be bent from the last maqam note. Thru won't
    // send a bend before the next note, so drop the offset now and keep
    // only the wheel. Zeroing noteBend also keeps our memory in step with
    // the synth for when we leave bypass.
    noteBend = 0;
    sendCombinedBend(lastNoteChannel);
    synth.passThrough(true);
  } else {
    synth.passThrough(false);
    retuneHeldNote();                       // a held key gets its maqam offset back
  }

  displaySetMaqam(maqams[maqam].name, tonic, bypass);
}


// ===== Tuning =====

// The only place a bend is sent: wheel + maqam offset (the math is in
// bend.cpp). Returns the value sent, for the display.
int16_t Engine::sendCombinedBend(uint8_t channel) {
  int16_t total = combineBend(wheelBend, noteBend);
  synth.bend(channel, total);
  return total;
}

// Send the bend for whichever key the synth is sounding, and report it
// to the display. Called whenever the held keys change.
void Engine::tuneSoundingNote(uint8_t channel) {
  uint8_t note  = heldNotes.sounding(settings.notePriority);
  int16_t cents = maqamCentsForNote(note, tonic, maqam);
  noteBend      = centsToBend(cents, settings.bendRangeCents);
  int16_t bend  = sendCombinedBend(channel);

  displaySetNote(note, cents, bend);
}

// A knob or bypass changed while a key may be held: retune it now rather
// than on the next note. This isn't triggered by a MIDI message, so there's
// no incoming channel; use the last note's.
void Engine::retuneHeldNote() {
  if (heldNotes.any() && !bypass) {
    tuneSoundingNote(lastNoteChannel);
  }
}
