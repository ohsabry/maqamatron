// Maqamatron
// Knob A0 picks the tonic, knob A1 picks the maqam.
// Shield switch: PROG to upload, RUN to play.

#include <MIDI.h>
#include <LiquidCrystal.h>

#include "maqams.h"

MIDI_CREATE_DEFAULT_INSTANCE();              // MIDI on hardware serial (D0/D1)
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);     // RS, E, D4, D5, D6, D7


// ============================================================
// Pins
// ============================================================

const int TONIC_KNOB_PIN    = A0;
const int MAQAM_KNOB_PIN    = A1;
const int BYPASS_BUTTON_PIN = 4;     // reads LOW while pressed
const int MAQAM_LED_PIN     = 6;     // green: lit while the maqam is active, like a pedal
const int SPARE_LED_PIN     = 7;     // red: not used yet, kept off

const int LED_ON  = LOW;             // the shield's LEDs are active low
const int LED_OFF = HIGH;


// ============================================================
// Settings
// ============================================================

const long BEND_RANGE_CENTS = 200;   // Common bend range: +/-2 semitones
const int  BEND_MIN = -8192;         // MIDI library bends are centered on 0
const int  BEND_MAX = 8191;

// A wheel resting a hair off centre counts as centred.
// 64 of 8192 is about 1.5 cents, too small to hear as a step.
const int WHEEL_DEAD_ZONE = 64;

// How many held keys we remember. Ten fingers; if more arrive, the oldest is dropped.
const int MAX_HELD_NOTES = 10;

const unsigned long BYPASS_BUTTON_DEBOUNCE_MS = 30;  // how long the contacts take to settle
const unsigned long DISPLAY_INTERVAL_MS = 100;        // at most 10 LCD redraws a second


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

// What LCD line 2 shows: the note we last tuned. Saved by
// tuneSoundingNote(), drawn later by updateDisplay().
bool noteHasBeenPlayed = false;      // line 2 stays blank until the first note
byte lastNote      = 0;
int  lastNoteCents = 0;
int  lastNoteBend  = 0;

// Dirty flag: "the LCD is out of date". Whatever changes what it shows
// sets the flag; updateDisplay() in loop() redraws at most every
// DISPLAY_INTERVAL_MS, so MIDI never waits for the slow LCD.
bool displayDirty = false;
unsigned long lastDisplayUpdateTime = 0;


// ============================================================
// Bend math
// ============================================================

// Turn cents into a pitch bend value (0 = no bend).
int centsToBend(int cents) {
  long bend = (long)cents * 8192 / BEND_RANGE_CENTS;   // long: 50 * 8192 overflows an int
  if (bend < BEND_MIN) bend = BEND_MIN;
  if (bend > BEND_MAX) bend = BEND_MAX;
  return (int)bend;
}

// The only place a bend gets sent: wheel + maqam offset, kept inside the
// legal range. Past the edges the offset gets cut off (e.g. a half-flat
// with the wheel fully down). Returns what was sent, for the LCD.
int sendCombinedBend(byte channel) {
  int total = wheelBend + noteBend;      // at most about +/-16000, fits in an int
  if (total < BEND_MIN) total = BEND_MIN;
  if (total > BEND_MAX) total = BEND_MAX;
  MIDI.sendPitchBend(total, channel);
  return total;
}


// ============================================================
// Display (LCD + LED)
// ============================================================

// Print spaces to fill out a field. Writing over old text with padding
// is cheaper than wiping the whole line first and then writing.
void printSpaces(int count) {
  for (int i = 0; i < count; i++) {
    lcd.print(' ');
  }
}

// LCD line 1: maqam name and tonic, e.g. "Husseini       D",
// or just "BYPASS" when bypassed. Always writes all 16 columns.
void drawMaqamLine() {
  lcd.setCursor(0, 0);
  if (bypass) {
    int written = lcd.print("BYPASS");      // print() returns how many characters it wrote
    printSpaces(16 - written);
    return;
  }
  int written = lcd.print(maqams[maqam].name);
  printSpaces(14 - written);                // name fills columns 0-13
  written = lcd.print(noteNames[tonic]);
  printSpaces(2 - written);                 // tonic fills columns 14-15
}

// LCD line 2: last note, its offset and the bend sent, e.g. "E4 -50c  b-2048".
// Always writes all 16 columns.
void drawNoteLine() {
  lcd.setCursor(0, 1);
  int written = 0;
  written += lcd.print(noteNames[lastNote % 12]);
  written += lcd.print(lastNote / 12 - 1); // octave (MIDI note 60 = C4)
  written += lcd.print(" ");
  written += lcd.print(lastNoteCents);
  written += lcd.print("c  b");
  written += lcd.print(lastNoteBend);
  printSpaces(16 - written);
}

// Called every loop(): redraw the LCD if it's out of date, but not more
// often than DISPLAY_INTERVAL_MS. Uses millis() like the debounce, so it
// never waits.
void updateDisplay() {
  if (!displayDirty) {
    return;                                 // nothing changed
  }
  if (millis() - lastDisplayUpdateTime < DISPLAY_INTERVAL_MS) {
    return;                                 // drew recently, try again later
  }

  drawMaqamLine();
  if (noteHasBeenPlayed) {
    drawNoteLine();
  }
  displayDirty = false;
  lastDisplayUpdateTime = millis();
}

// Light the maqam LED unless bypassed.
void showMaqamLed() {
  if (bypass) {
    digitalWrite(MAQAM_LED_PIN, LED_OFF);
  } else {
    digitalWrite(MAQAM_LED_PIN, LED_ON);
  }
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
    displayDirty = true;
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

  displayDirty = true;
  showMaqamLed();                           // an LED is instant, no need to wait
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
  noteBend   = centsToBend(cents);
  int  bend  = sendCombinedBend(channel);

  lastNote      = note;                     // for LCD line 2, drawn later
  lastNoteCents = cents;
  lastNoteBend  = bend;
  noteHasBeenPlayed = true;
  displayDirty = true;
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
  if (abs(bend) < WHEEL_DEAD_ZONE) {
    bend = 0;
  }
  wheelBend = bend;                         // track it even in bypass...

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
  lcd.begin(16, 2);
  pinMode(BYPASS_BUTTON_PIN, INPUT_PULLUP); // internal resistor holds the pin HIGH until pressed
  pinMode(MAQAM_LED_PIN, OUTPUT);
  pinMode(SPARE_LED_PIN, OUTPUT);
  digitalWrite(SPARE_LED_PIN, LED_OFF);
  showMaqamLed();
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
  updateDisplay();
}
