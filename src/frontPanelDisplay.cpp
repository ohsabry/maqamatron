#include "frontPanelDisplay.h"

#include <Arduino.h>

#include "maqams.h"

static const int LCD_RS = 13, LCD_E = 12, LCD_D4 = 11, LCD_D5 = 10, LCD_D6 = 9, LCD_D7 = 8;

static const int MAQAM_LED_PIN  = 6;     // green: lit unless bypassed
static const int UNUSED_LED_PIN = 7;     // red
static const int LED_ON  = LOW;          // the shield's LEDs are active low
static const int LED_OFF = HIGH;

static const unsigned long UPDATE_INTERVAL_MS = 100;

FrontPanelDisplay::FrontPanelDisplay()
  : lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7) {}

void FrontPanelDisplay::begin() {
  lcd.begin(16, 2);
  pinMode(MAQAM_LED_PIN, OUTPUT);

  // A floating pin can make the unused LED glow faintly, so drive it off.
  // Writing HIGH before switching to output means it never flashes on.
  digitalWrite(UNUSED_LED_PIN, LED_OFF);
  pinMode(UNUSED_LED_PIN, OUTPUT);
}

void FrontPanelDisplay::showMaqam(const char* maqamName, int16_t tonic, bool bypass) {
  shownMaqamName = maqamName;
  shownTonic     = tonic;
  shownBypass    = bypass;
  dirty = true;
}

void FrontPanelDisplay::showNote(uint8_t note, int16_t cents, int16_t bend) {
  shownNote  = note;
  shownCents = cents;
  shownBend  = bend;
  noteHasBeenPlayed = true;
  dirty = true;
}

void FrontPanelDisplay::update() {
  if (!dirty) {
    return;
  }

  // The LED is fast, so it doesn't wait for the LCD's rate limit.
  showMaqamLed();

  if (millis() - lastUpdateTime < UPDATE_INTERVAL_MS) {
    return;
  }

  drawMaqamLine();
  if (noteHasBeenPlayed) {
    drawNoteLine();
  }
  dirty = false;
  lastUpdateTime = millis();
}

// Padding over old text is faster than clearing the line first.
void FrontPanelDisplay::printSpaces(int16_t count) {
  for (int16_t i = 0; i < count; i++) {
    lcd.print(' ');
  }
}

// "Husseini       D", or "BYPASS".
void FrontPanelDisplay::drawMaqamLine() {
  lcd.setCursor(0, 0);
  if (shownBypass) {
    int16_t written = lcd.print("BYPASS");
    printSpaces(16 - written);
    return;
  }
  int16_t written = lcd.print(shownMaqamName);
  printSpaces(14 - written);
  written = lcd.print(noteNames[shownTonic]);
  printSpaces(2 - written);
}

// "E4 -50c  b-2048"
void FrontPanelDisplay::drawNoteLine() {
  lcd.setCursor(0, 1);
  int16_t written = 0;
  written += lcd.print(noteNames[shownNote % 12]);
  written += lcd.print(shownNote / 12 - 1); // MIDI note 60 is C4
  written += lcd.print(" ");
  written += lcd.print(shownCents);
  written += lcd.print("c  b");
  written += lcd.print(shownBend);
  printSpaces(16 - written);
}

void FrontPanelDisplay::showMaqamLed() {
  digitalWrite(MAQAM_LED_PIN, shownBypass ? LED_OFF : LED_ON);
}
