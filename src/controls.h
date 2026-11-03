// The tonic knob, maqam knob and bypass button. Reports what changed;
// what a change means is up to the caller.

#pragma once

#include <stdint.h>

void controlsBegin(int16_t maqamCount);

// Call every loop(), before the functions below.
void controlsUpdate();

// Knob positions: tonic 0-11, maqam 0 to maqamCount - 1.
int16_t controlsTonic();
int16_t controlsMaqam();

// True only for the update in which it happened.
bool controlsKnobsChanged();
bool controlsBypassPressed();
