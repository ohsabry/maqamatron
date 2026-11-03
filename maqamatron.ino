// Maqamatron: Arabic maqam quarter tones for a mono synth.
//
// Sits between a MIDI keyboard and a synth. Each note gets a pitch bend
// that puts it where the selected maqam wants it (e.g. E half-flat in
// Bayati), with the keyboard's own pitch wheel added on top. Everything
// else passes through untouched.
//
// Hardware: Arduino Uno (or SparkFun RedBoard) with the SparkFun MIDI
// Shield and a 16x2 LCD. Set the shield's switch to PROG to upload and
// RUN to play.
//
// This file is the wiring: it connects the MIDI port, knobs and button to
// the Engine (src/engine.h), which does the actual tuning.

#include <MIDI.h>

#include "src/settings.h"
#include "src/maqams.h"
#include "src/display.h"
#include "src/controls.h"
#include "src/synth.h"
#include "src/engine.h"

MIDI_CREATE_DEFAULT_INSTANCE();              // MIDI on hardware serial (D0/D1)


// ============================================================
// The synth, reached over MIDI
// ============================================================

class MidiSynth : public Synth {
public:
  void noteOn(uint8_t channel, uint8_t note, uint8_t velocity) override {
    MIDI.sendNoteOn(note, velocity, channel);
  }
  void noteOff(uint8_t channel, uint8_t note, uint8_t velocity) override {
    MIDI.sendNoteOff(note, velocity, channel);
  }
  void bend(uint8_t channel, int16_t bend) override {
    MIDI.sendPitchBend(bend, channel);
  }
  void passThrough(bool on) override {
    if (on) {
      MIDI.turnThruOn();
    } else {
      MIDI.turnThruOff();
    }
  }
};

MidiSynth synth;
Engine engine(synth, { BEND_RANGE_CENTS, WHEEL_DEAD_ZONE, NOTE_PRIORITY });


// ============================================================
// Controls
// ============================================================

void readControls() {
  controlsUpdate();

  if (controlsKnobsChanged()) {
    engine.setMaqam(controlsTonic(), controlsMaqam());
  }
  if (controlsBypassPressed()) {
    engine.toggleBypass();
  }
}


// ============================================================
// MIDI in
// ============================================================

// The MIDI library calls plain functions, so these hand each message on
// to the engine.
void handleNoteOn(byte channel, byte note, byte velocity)  { engine.noteOn(channel, note, velocity); }
void handleNoteOff(byte channel, byte note, byte velocity) { engine.noteOff(channel, note, velocity); }
void handlePitchBend(byte channel, int bend)               { engine.wheel(channel, bend); }

// A filtered thru: send the message MIDI.read() just received back out
// unchanged, unless the engine handles it or it's on the dropped list below.
void forwardOtherMessage() {
  midi::MidiType type = MIDI.getType();

  switch (type) {
    // Handled by the engine: notes get bent, and the wheel is
    // combined with the maqam offset.
    case midi::NoteOn:
    case midi::NoteOff:
    case midi::PitchBend:
      break;

    // Channel messages: type, up to two data bytes, channel.
    case midi::ControlChange:               // incl. mod wheel (CC 1)
    case midi::ProgramChange:
    case midi::AfterTouchChannel:
    case midi::AfterTouchPoly:
      MIDI.send(type, MIDI.getData1(), MIDI.getData2(), MIDI.getChannel());
      break;

    // Real-time and other one-byte messages: just the type.
    case midi::Clock:
    case midi::Start:
    case midi::Continue:
    case midi::Stop:
    case midi::ActiveSensing:
    case midi::SystemReset:
    case midi::TuneRequest:
      MIDI.sendRealTime(type);
      break;

    // System exclusive: a block of any length. The stored bytes
    // already include the F0 start and F7 end, hence true.
    case midi::SystemExclusive:
      MIDI.sendSysEx(MIDI.getSysExArrayLength(), MIDI.getSysExArray(), true);
      break;

    // Everything else is dropped: Song Position, Song Select and MIDI
    // timecode aren't needed for a keyboard-to-synth setup.
    default:
      break;
  }
}


// ============================================================
// setup / loop
// ============================================================

void setup() {
  displayBegin();
  controlsBegin(maqamCount);

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.begin(MIDI_CHANNEL_OMNI);            // listen on all channels
  MIDI.turnThruOff();                       // we forward messages ourselves

  // Know the tonic and maqam before any note arrives. After MIDI.begin(),
  // because a button held at power-on toggles bypass, which sends MIDI.
  readControls();
}

void loop() {
  // read() calls the handlers above, then returns true if a message
  // arrived. In bypass, the library's thru has already sent it.
  if (MIDI.read() && !engine.bypassed()) {
    forwardOtherMessage();
  }
  readControls();
  displayUpdate();
}
