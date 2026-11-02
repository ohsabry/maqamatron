// Maqamatron
// Knob A0 picks the tonic, knob A1 picks the maqam.
// Shield switch: PROG to upload, RUN to play.

#include <MIDI.h>

#include "settings.h"
#include "maqams.h"
#include "bend.h"
#include "display.h"
#include "controls.h"
#include "heldNotes.h"

MIDI_CREATE_DEFAULT_INSTANCE();              // MIDI on hardware serial (D0/D1)


// ============================================================
// State
// ============================================================

// Current settings, set by the knobs. -1 means "not read yet",
// so the first knob reading always counts as a change and draws the LCD.
int tonic = -1;
int maqam = -1;

bool bypass = false;                 // toggled by the bypass button

// The synth has one bend setting, fed by two sources. We remember both
// and always send their sum: what you hear = key + maqam offset + wheel.
int  wheelBend = 0;                  // where the keyboard's pitch wheel is (0 = centred)
int  noteBend  = 0;                  // maqam offset of the note currently sounding
byte lastNoteChannel = 1;            // channel of the last note played, for bends no MIDI message triggered

// Keys currently held, in the order pressed (see heldNotes.h).
HeldNotes heldNotes;



// ============================================================
// Sending bends
// ============================================================

// The only place a bend gets sent: wheel + maqam offset (the math is in
// bend.cpp). Returns what was sent, for the LCD.
int sendCombinedBend(byte channel) {
  int total = combineBend(wheelBend, noteBend);
  MIDI.sendPitchBend(total, channel);
  return total;
}


// ============================================================
// Controls: what knob and button changes mean
// ============================================================

// Read the knobs and button (in controls.cpp), and decide what changes mean.
void readControls() {
  controlsUpdate();

  if (controlsKnobsChanged()) {
    tonic = controlsTonic();
    maqam = controlsMaqam();
    displaySetMaqam(maqams[maqam].name, tonic, bypass);
    retuneHeldNote();                       // a held key takes the new tuning now
  }
  if (controlsBypassPressed()) {
    toggleBypass();
  }
}

// True bypass: in bypass the library's thru copies every incoming message
// to the output untouched, and our note handlers stay out of the way.
void toggleBypass() {
  bypass = !bypass;

  if (bypass) {
    // The synth may still be bent from the last maqam note. Thru won't
    // send a bend before the next note, so drop the offset now and keep
    // only the wheel. Zeroing noteBend also keeps our memory in step with
    // the synth for when we leave bypass.
    noteBend = 0;
    sendCombinedBend(lastNoteChannel);
    MIDI.turnThruOn();
  } else {
    MIDI.turnThruOff();
    retuneHeldNote();                       // a held key gets its maqam offset back
  }

  displaySetMaqam(maqams[maqam].name, tonic, bypass);
}


// ============================================================
// Tuning the held notes
// ============================================================

// Send the bend for whichever key the synth is sounding, and note it
// for the LCD. Called whenever the held keys change.
void tuneSoundingNote(byte channel) {
  byte note  = heldNotes.sounding(NOTE_PRIORITY);
  int  cents = maqamCentsForNote(note, tonic, maqam);
  noteBend   = centsToBend(cents, BEND_RANGE_CENTS);
  int  bend  = sendCombinedBend(channel);

  displaySetNote(note, cents, bend);
}

// A knob or bypass changed, and a key may still be held: retune it now
// instead of waiting for the next note. No MIDI message came in, so
// there's no channel to reply on; use the last note's.
void retuneHeldNote() {
  if (heldNotes.any() && !bypass) {
    tuneSoundingNote(lastNoteChannel);
  }
}


// ============================================================
// MIDI handlers
// ============================================================

// The held-keys list is kept up to date even in bypass, so it's
// right when we leave bypass.

void handleNoteOn(byte channel, byte note, byte velocity) {
  heldNotes.add(note);
  lastNoteChannel = channel;
  if (bypass) {
    return;                                 // thru already forwarded it
  }

  tuneSoundingNote(channel);                // bend first (offset + wheel)...
  MIDI.sendNoteOn(note, velocity, channel); // ...then the note
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  heldNotes.remove(note);
  if (bypass) {
    return;                                 // thru already forwarded it
  }

  MIDI.sendNoteOff(note, velocity, channel);

  // If keys are still held, the synth falls back to one of them:
  // send its bend. (If an older key was released, the sounding key
  // hasn't changed and this just re-sends the same bend.)
  if (heldNotes.any()) {
    tuneSoundingNote(channel);
  }
}

// The keyboard's pitch wheel moved. bend is -8192..8191, 0 = centred.
// No LCD here: a moving wheel sends many messages a second, and drawing
// for each one would slow MIDI down.
void handlePitchBend(byte channel, int bend) {
  wheelBend = applyWheelDeadZone(bend, WHEEL_DEAD_ZONE);   // track it even in bypass...

  if (bypass) {
    return;                                 // ...but thru already forwarded it
  }
  sendCombinedBend(channel);
}

// Our own thru with a filter: send the message MIDI.read() just received
// back out unchanged, unless it's one we handle ourselves.
// Mirrors how the library's built-in thru sends each kind of message.
void forwardOtherMessage() {
  midi::MidiType type = MIDI.getType();

  switch (type) {
    // Handled elsewhere: notes are bent by our handlers,
    // handlePitchBend combines the wheel with the maqam offset.
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

    // Everything else is dropped, incl. Song Position, Song Select and
    // timecode.
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
  // read() calls our note handlers, then returns true if a message arrived.
  // In bypass, the library's thru has already sent it.
  if (MIDI.read() && !bypass) {
    forwardOtherMessage();
  }
  readControls();
  displayUpdate();
}
