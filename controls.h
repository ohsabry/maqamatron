// What the player touches: the tonic knob, the maqam knob and the bypass button.
//
// The controls only read and report. controlsUpdate() does the hardware
// work once per loop; the other functions answer questions about what it
// found. What a change *means* is up to the rest of the code.

#pragma once

#include <stdint.h>

// Call once in setup(). maqamCount: how many slices to cut the maqam knob into.
void controlsBegin(int16_t maqamCount);

// Call every loop(), before asking the questions below.
void controlsUpdate();

// Where the knobs are, as slices: tonic 0-11, maqam 0 to maqamCount - 1.
int16_t controlsTonic();
int16_t controlsMaqam();

// Events: true only for the update in which they happened.
bool controlsKnobsChanged();     // either knob moved into a new slice
bool controlsBypassPressed();    // the bypass button was just pressed (debounced)
