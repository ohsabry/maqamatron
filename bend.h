// Pitch bend math: turning cents into bend values, and combining bends.
// Pure math. No hardware, no MIDI library, so it works on any chip.

#pragma once

// MIDI facts: a pitch bend is 14 bits, centred on 0 in these units.
const int BEND_MIN = -8192;
const int BEND_MAX = 8191;

// Turn cents into a bend value (0 = no bend).
// rangeCents: how far a full bend goes on the synth, e.g. 200 for +/-2 semitones.
int centsToBend(int cents, int rangeCents);

// Wheel + maqam offset, kept inside BEND_MIN..BEND_MAX. Past the edges the
// offset gets cut off (e.g. a half-flat with the wheel fully down).
int combineBend(int wheelBend, int noteBend);

// A wheel within deadZone of centre counts as exactly centred (0).
int applyWheelDeadZone(int wheelBend, int deadZone);
