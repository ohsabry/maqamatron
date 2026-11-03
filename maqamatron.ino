// Maqamatron: retunes a mono synth to Arabic maqam quarter tones.
//
// Sits between a MIDI keyboard and a synth. Before each note it sends a
// pitch bend that puts the note where the selected maqam wants it, with the
// keyboard's pitch wheel added on top. Other MIDI passes through.
//
// This file wires the MIDI port, knobs and button to the Engine
// (src/engine.h), which does the tuning.

#include <MIDI.h>

#include "src/settings.h"
#include "src/maqams.h"
#include "src/frontPanelDisplay.h"
#include "src/controls.h"
#include "src/synth.h"
#include "src/engine.h"

MIDI_CREATE_DEFAULT_INSTANCE();

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
FrontPanelDisplay display;
Engine engine(synth, display, { BEND_RANGE_CENTS, WHEEL_DEAD_ZONE, NOTE_PRIORITY });

void readControls() {
  controlsUpdate();

  if (controlsKnobsChanged()) {
    engine.setMaqam(controlsTonic(), controlsMaqam());
  }
  if (controlsBypassPressed()) {
    engine.toggleBypass();
  }
}

// The MIDI library takes plain functions, not member functions.
void handleNoteOn(byte channel, byte note, byte velocity)  { engine.noteOn(channel, note, velocity); }
void handleNoteOff(byte channel, byte note, byte velocity) { engine.noteOff(channel, note, velocity); }
void handlePitchBend(byte channel, int bend)               { engine.wheel(channel, bend); }

// Forwards the message MIDI.read() just received, unless the engine handles
// it. Song Position, Song Select and MIDI timecode aren't needed between a
// keyboard and a synth, so they're dropped.
void forwardOtherMessage() {
  midi::MidiType type = MIDI.getType();

  switch (type) {
    case midi::NoteOn:
    case midi::NoteOff:
    case midi::PitchBend:
      break;

    case midi::ControlChange:
    case midi::ProgramChange:
    case midi::AfterTouchChannel:
    case midi::AfterTouchPoly:
      MIDI.send(type, MIDI.getData1(), MIDI.getData2(), MIDI.getChannel());
      break;

    case midi::Clock:
    case midi::Start:
    case midi::Continue:
    case midi::Stop:
    case midi::ActiveSensing:
    case midi::SystemReset:
    case midi::TuneRequest:
      MIDI.sendRealTime(type);
      break;

    case midi::SystemExclusive:
      // The stored message already includes the F0 and F7 bytes.
      MIDI.sendSysEx(MIDI.getSysExArrayLength(), MIDI.getSysExArray(), true);
      break;

    default:
      break;
  }
}

void setup() {
  display.begin();
  controlsBegin(maqamCount);

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();                       // forwardOtherMessage() forwards instead

  // After MIDI.begin(): a button held at power-on toggles bypass, which sends MIDI.
  readControls();
}

void loop() {
  // In bypass, the library's thru has already forwarded the message.
  if (MIDI.read() && !engine.bypassed()) {
    forwardOtherMessage();
  }
  readControls();
  display.update();
}
