#include "controls.h"

#include <Arduino.h>

static const int TONIC_KNOB_PIN    = A0;
static const int MAQAM_KNOB_PIN    = A1;
static const int BYPASS_BUTTON_PIN = 4;     // LOW while pressed

static const unsigned long BYPASS_BUTTON_DEBOUNCE_MS = 30;

static int16_t maqamSlices = 1;

// -1 so the first update always counts as a change.
static int16_t tonicSlice = -1;
static int16_t maqamSlice = -1;

static int lastBypassButtonReading = HIGH;
static unsigned long lastBypassButtonChangeTime = 0;

static bool knobsChanged  = false;
static bool bypassPressed = false;

// 1024, not 1023, so the top slice is as wide as the others.
int16_t controlsTonicKnob(int16_t slices) {
  return map(analogRead(TONIC_KNOB_PIN), 0, 1024, 0, slices);
}

int16_t controlsMaqamKnob(int16_t slices) {
  return map(analogRead(MAQAM_KNOB_PIN), 0, 1024, 0, slices);
}

bool controlsBypassHeld() {
  return digitalRead(BYPASS_BUTTON_PIN) == LOW;
}

static void readKnobs() {
  int16_t newTonic = controlsTonicKnob(12);
  int16_t newMaqam = controlsMaqamKnob(maqamSlices);

  if (newTonic != tonicSlice || newMaqam != maqamSlice) {
    tonicSlice = newTonic;
    maqamSlice = newMaqam;
    knobsChanged = true;
  }
}

// Reports the moment of pressing, not holding. After any change the pin is
// ignored while the contacts bounce; no delay(), so MIDI keeps flowing.
static void readBypassButton() {
  if (millis() - lastBypassButtonChangeTime < BYPASS_BUTTON_DEBOUNCE_MS) {
    return;
  }

  int reading = digitalRead(BYPASS_BUTTON_PIN);
  if (reading == lastBypassButtonReading) {
    return;
  }

  lastBypassButtonChangeTime = millis();
  lastBypassButtonReading = reading;

  if (reading == LOW) {
    bypassPressed = true;
  }
}

void controlsBegin(int16_t maqamCount) {
  maqamSlices = maqamCount;
  pinMode(BYPASS_BUTTON_PIN, INPUT_PULLUP); // internal resistor holds the pin HIGH until pressed
}

void controlsUpdate() {
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
