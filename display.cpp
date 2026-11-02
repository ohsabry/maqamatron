#include "display.h"

#include <Arduino.h>          // millis()
#include <LiquidCrystal.h>

#include "maqams.h"           // noteNames

// Everything below is "static": private to this file. The rest of the
// code can only reach the display through the functions in display.h.

static LiquidCrystal lcd(13, 12, 11, 10, 9, 8);     // RS, E, D4, D5, D6, D7

static const int MAQAM_LED_PIN = 6;      // green: lit while the maqam is active, like a pedal
static const int UNUSED_LED_PIN = 7;     // red: not used, held off (see displayBegin)
static const int LED_ON  = LOW;          // the shield's LEDs are active low
static const int LED_OFF = HIGH;

static const unsigned long DISPLAY_INTERVAL_MS = 100;  // at most 10 redraws a second

// What to show, as last reported by displaySet...().
static const char* shownMaqamName = "";
static int16_t shownTonic  = 0;
static bool    shownBypass = false;

static bool    noteHasBeenPlayed = false;   // line 2 stays blank until the first note
static uint8_t shownNote  = 0;
static int16_t shownCents = 0;
static int16_t shownBend  = 0;

// Dirty flag: "the LCD is out of date". Set by displaySet...(),
// cleared by displayUpdate() once it has redrawn.
static bool displayDirty = false;
static unsigned long lastDisplayUpdateTime = 0;

// Print spaces to fill out a field. Writing over old text with padding
// is cheaper than wiping the whole line first and then writing.
static void printSpaces(int16_t count) {
  for (int16_t i = 0; i < count; i++) {
    lcd.print(' ');
  }
}

// Line 1: maqam name and tonic, e.g. "Husseini       D",
// or just "BYPASS" when bypassed. Always writes all 16 columns.
static void drawMaqamLine() {
  lcd.setCursor(0, 0);
  if (shownBypass) {
    int16_t written = lcd.print("BYPASS");  // print() returns how many characters it wrote
    printSpaces(16 - written);
    return;
  }
  int16_t written = lcd.print(shownMaqamName);
  printSpaces(14 - written);                // name fills columns 0-13
  written = lcd.print(noteNames[shownTonic]);
  printSpaces(2 - written);                 // tonic fills columns 14-15
}

// Line 2: last note, its offset and the bend sent, e.g. "E4 -50c  b-2048".
// Always writes all 16 columns.
static void drawNoteLine() {
  lcd.setCursor(0, 1);
  int16_t written = 0;
  written += lcd.print(noteNames[shownNote % 12]);
  written += lcd.print(shownNote / 12 - 1); // octave (MIDI note 60 = C4)
  written += lcd.print(" ");
  written += lcd.print(shownCents);
  written += lcd.print("c  b");
  written += lcd.print(shownBend);
  printSpaces(16 - written);
}

// Green LED: lit unless bypassed.
static void showMaqamLed() {
  if (shownBypass) {
    digitalWrite(MAQAM_LED_PIN, LED_OFF);
  } else {
    digitalWrite(MAQAM_LED_PIN, LED_ON);
  }
}

void displayBegin() {
  lcd.begin(16, 2);
  pinMode(MAQAM_LED_PIN, OUTPUT);

  // The red LED isn't used, but drive it off on purpose: a pin left as an
  // unconnected input can let a faint glow through. Writing HIGH before
  // switching to output means it never flashes on, not even for a moment.
  digitalWrite(UNUSED_LED_PIN, LED_OFF);
  pinMode(UNUSED_LED_PIN, OUTPUT);
}

void displaySetMaqam(const char* maqamName, int16_t tonic, bool bypass) {
  shownMaqamName = maqamName;
  shownTonic     = tonic;
  shownBypass    = bypass;
  displayDirty   = true;
}

void displaySetNote(uint8_t note, int16_t cents, int16_t bend) {
  shownNote  = note;
  shownCents = cents;
  shownBend  = bend;
  noteHasBeenPlayed = true;
  displayDirty = true;
}

void displayUpdate() {
  if (!displayDirty) {
    return;                                 // nothing changed
  }

  // The LED is instant, so it doesn't wait for the LCD's 100 ms limit.
  showMaqamLed();

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
