// What the player sees: the LCD (maqam and tonic on line 1, the last
// tuned note on line 2) and the maqam LED (lit unless bypassed).
//
// "Tell, don't draw": the rest of the code reports what's true with the
// displaySet...() functions, which are instant. The display decides when
// to actually redraw, in displayUpdate(), so MIDI never waits for the
// slow LCD.

#pragma once

#include <stdint.h>

// Call once in setup().
void displayBegin();

// "This is the current maqam, tonic and bypass state."
void displaySetMaqam(const char* maqamName, int16_t tonic, bool bypass);

// "This note was just tuned: this offset, this bend sent."
void displaySetNote(uint8_t note, int16_t cents, int16_t bend);

// Call every loop(). Shows whatever changed: the LED right away, the LCD
// at most 10 times a second.
void displayUpdate();
