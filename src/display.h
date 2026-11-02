// What the player sees: the LCD (maqam and tonic on line 1, the last
// tuned note on line 2) and the maqam LED (lit unless bypassed).
//
// The rest of the code reports state with the displaySet...() functions,
// which only record it. displayUpdate() decides when to actually redraw,
// so MIDI handling never waits on the slow LCD.

#pragma once

#include <stdint.h>

// Call once in setup().
void displayBegin();

// Report the current maqam, tonic and bypass state.
void displaySetMaqam(const char* maqamName, int16_t tonic, bool bypass);

// Report a note that was just tuned: its offset in cents and the bend sent.
void displaySetNote(uint8_t note, int16_t cents, int16_t bend);

// Call every loop(). Shows whatever changed: the LED right away, the LCD
// at most 10 times a second.
void displayUpdate();
