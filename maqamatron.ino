// Maqamatron
// Knob A0 picks the tonic, knob A1 picks the maqam.
// Shield switch: PROG to upload, RUN to play.

#include <MIDI.h>

#include "settings.h"
#include "maqams.h"
#include "bend.h"
#include "display.h"

MIDI_CREATE_DEFAULT_INSTANCE();              // MIDI on hardware serial (D0/D1)


// ============================================================
// Pins
// ============================================================

const int TONIC_KNOB_PIN    = A0;
const int MAQAM_KNOB_PIN    = A1;
const int BYPASS_BUTTON_PIN = 4;     // reads LOW while pressed


// ============================================================
// Settings
// ============================================================

// How many held keys we remember. Ten fingers; if more arrive, the oldest is dropped.
const int MAX_HELD_NOTES = 10;

const unsigned long BYPASS_BUTTON_DEBOUNCE_MS = 30;  // how long the contacts take to settle


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

// Keys currently held, oldest first. A mono synth remembers held keys
// and falls back to one when the sounding key is released; we keep the
// same list so we know which note to send the bend for.
byte heldNotes[MAX_HELD_NOTES];
int  heldNoteCount = 0;

// Bypass button debouncing.
int  lastBypassButtonReading = HIGH; // HIGH = not pressed (pull-up)
unsigned long lastBypassButtonChangeTime = 0;  // millis() when the reading last changed


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
// Controls (knobs + bypass button)
// ============================================================

// Read both knobs. Each knob's 0..1023 range is cut into equal slices.
// map(x, 0, 1024, ...) rather than 1023, so the top slice is as wide as the others.
void readKnobs() {
  int newTonic = map(analogRead(TONIC_KNOB_PIN), 0, 1024, 0, 12);
  int newMaqam = map(analogRead(MAQAM_KNOB_PIN), 0, 1024, 0, maqamCount);

  if (newTonic != tonic || newMaqam != maqam) {
    tonic = newTonic;
    maqam = newMaqam;
    displaySetMaqam(maqams[maqam].name, tonic, bypass);
    retuneHeldNote();                       // a held key takes the new tuning now
  }
}

// Toggle bypass on the "edge" where the button goes from not pressed (HIGH)
// to pressed (LOW). Holding the button down does nothing more.
// Debounced: after any change, ignore the pin for
// BYPASS_BUTTON_DEBOUNCE_MS while the contacts stop bouncing.
// No delay(), so MIDI keeps flowing meanwhile.
void readBypassButton() {
  if (millis() - lastBypassButtonChangeTime < BYPASS_BUTTON_DEBOUNCE_MS) {
    return;                                                  // still settling
  }

  int reading = digitalRead(BYPASS_BUTTON_PIN);
  if (reading == lastBypassButtonReading) {
    return;                                                  // no change
  }

  lastBypassButtonChangeTime = millis();
  lastBypassButtonReading = reading;

  if (reading == LOW) {                                      // just pressed
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
// Held notes
// ============================================================

// Take a key off the list, if it's there, and close the gap.
void removeHeldNote(byte note) {
  for (int i = 0; i < heldNoteCount; i++) {
    if (heldNotes[i] == note) {
      // Shift every later key one slot down, over the removed one.
      for (int j = i; j < heldNoteCount - 1; j++) {
        heldNotes[j] = heldNotes[j + 1];
      }
      heldNoteCount--;
      return;
    }
  }
}

// Put a key on the end of the list (newest).
void addHeldNote(byte note) {
  removeHeldNote(note);                     // never list the same key twice
  if (heldNoteCount == MAX_HELD_NOTES) {
    removeHeldNote(heldNotes[0]);           // full: drop the oldest
  }
  heldNotes[heldNoteCount] = note;
  heldNoteCount++;
}

// Which held key the synth is sounding. Must match the synth's
// note priority setting. Only call when heldNoteCount > 0.
//   Last: the most recently pressed key wins. Implemented.
//   Low:  the lowest held key wins. Not implemented.
//   High: the highest held key wins. Not implemented.
byte soundingNote() {
  return heldNotes[heldNoteCount - 1];      // Last: the newest key
}

// Send the bend for whichever key the synth is sounding, and note it
// for the LCD. Called whenever the held keys change.
void tuneSoundingNote(byte channel) {
  byte note  = soundingNote();
  int  cents = maqamCentsForNote(note, tonic, maqam);
  noteBend   = centsToBend(cents, BEND_RANGE_CENTS);
  int  bend  = sendCombinedBend(channel);

  displaySetNote(note, cents, bend);
}

// A knob or bypass changed, and a key may still be held: retune it now
// instead of waiting for the next note. No MIDI message came in, so
// there's no channel to reply on; use the last note's.
void retuneHeldNote() {
  if (heldNoteCount > 0 && !bypass) {
    tuneSoundingNote(lastNoteChannel);
  }
}


// ============================================================
// MIDI handlers
// ============================================================

// The held-keys list is kept up to date even in bypass, so it's
// right when we leave bypass.

void handleNoteOn(byte channel, byte note, byte velocity) {
  addHeldNote(note);
  lastNoteChannel = channel;
  if (bypass) {
    return;                                 // thru already forwarded it
  }

  tuneSoundingNote(channel);                // bend first (offset + wheel)...
  MIDI.sendNoteOn(note, velocity, channel); // ...then the note
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  removeHeldNote(note);
  if (bypass) {
    return;                                 // thru already forwarded it
  }

  MIDI.sendNoteOff(note, velocity, channel);

  // If keys are still held, the synth falls back to one of them:
  // send its bend. (If an older key was released, the sounding key
  // hasn't changed and this just re-sends the same bend.)
  if (heldNoteCount > 0) {
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
  pinMode(BYPASS_BUTTON_PIN, INPUT_PULLUP); // internal resistor holds the pin HIGH until pressed
  readKnobs();                              // know the tonic and maqam before any note arrives

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.begin(MIDI_CHANNEL_OMNI);            // listen on all channels
  MIDI.turnThruOff();                       // we forward messages ourselves
}

void loop() {
  // read() calls our note handlers, then returns true if a message arrived.
  // In bypass, the library's thru has already sent it.
  if (MIDI.read() && !bypass) {
    forwardOtherMessage();
  }
  readKnobs();
  readBypassButton();
  displayUpdate();
}
