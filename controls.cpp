#include "controls.h"

#include <Arduino.h>          // analogRead, digitalRead, millis

// Everything below is "static": private to this file. The rest of the
// code can only reach the controls through the functions in controls.h.

static const int TONIC_KNOB_PIN    = A0;
static const int MAQAM_KNOB_PIN    = A1;
static const int BYPASS_BUTTON_PIN = 4;     // reads LOW while pressed

static const unsigned long BYPASS_BUTTON_DEBOUNCE_MS = 30;  // how long the contacts take to settle

static int16_t maqamSlices = 1;             // set by controlsBegin()

// Knob slices. -1 means "not read yet", so the first update always
// counts as a change.
static int16_t tonicSlice = -1;
static int16_t maqamSlice = -1;

// Bypass button debouncing.
static int lastBypassButtonReading = HIGH;               // HIGH = not pressed (pull-up)
static unsigned long lastBypassButtonChangeTime = 0;     // millis() when the reading last changed

// This update's events.
static bool knobsChanged  = false;
static bool bypassPressed = false;

// Read both knobs. Each knob's 0..1023 range is cut into equal slices.
// map(x, 0, 1024, ...) rather than 1023, so the top slice is as wide as the others.
static void readKnobs() {
  int16_t newTonic = map(analogRead(TONIC_KNOB_PIN), 0, 1024, 0, 12);
  int16_t newMaqam = map(analogRead(MAQAM_KNOB_PIN), 0, 1024, 0, maqamSlices);

  if (newTonic != tonicSlice || newMaqam != maqamSlice) {
    tonicSlice = newTonic;
    maqamSlice = newMaqam;
    knobsChanged = true;
  }
}

// Spot the "edge" where the button goes from not pressed (HIGH) to
// pressed (LOW). Holding the button down does nothing more.
// Debounced: after any change, ignore the pin for
// BYPASS_BUTTON_DEBOUNCE_MS while the contacts stop bouncing.
// No delay(), so MIDI keeps flowing meanwhile.
static void readBypassButton() {
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
    bypassPressed = true;
  }
}

void controlsBegin(int16_t maqamCount) {
  maqamSlices = maqamCount;
  pinMode(BYPASS_BUTTON_PIN, INPUT_PULLUP); // internal resistor holds the pin HIGH until pressed
}

void controlsUpdate() {
  // Events only last one update: forget the last ones, then look again.
  knobsChanged  = false;
  bypassPressed = false;

  readKnobs();
  readBypassButton();
}

int16_t controlsTonic() {
  return tonicSlice;
}

int16_t controlsMaqam() {
  return maqamSlice;
}

bool controlsKnobsChanged() {
  return knobsChanged;
}

bool controlsBypassPressed() {
  return bypassPressed;
}
