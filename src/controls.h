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

// Which of `slices` equal parts each knob is in, read now. For the
// settings menu, which divides the knobs its own way.
int16_t controlsTonicKnob(int16_t slices);
int16_t controlsMaqamKnob(int16_t slices);

// Whether the button is down right now. For checking at power-on.
bool controlsBypassHeld();

// True only for the update in which it happened.
bool controlsKnobsChanged();
bool controlsBypassPressed();
