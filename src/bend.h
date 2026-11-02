// Pitch bend math: turning cents into bend values, and combining bends.
// Pure math. No hardware, no MIDI library, so it works on any chip.

#pragma once

#include <stdint.h>

// Fixed-width types match the Uno (int16_t = int, int32_t = long), so the
// code behaves the same on any chip and in tests on a computer.

// MIDI pitch bend is 14 bits; the MIDI library centres it on 0.
const int16_t BEND_MIN = -8192;
const int16_t BEND_MAX = 8191;

// Turn cents into a bend value (0 = no bend).
// rangeCents: how far a full bend goes on the synth, e.g. 200 for +/-2 semitones.
int16_t centsToBend(int16_t cents, int16_t rangeCents);

// Wheel + maqam offset, kept inside BEND_MIN..BEND_MAX. Past the edges the
// offset gets cut off (e.g. a half-flat with the wheel fully down).
int16_t combineBend(int16_t wheelBend, int16_t noteBend);

// A wheel within deadZone of centre counts as exactly centred (0).
int16_t applyWheelDeadZone(int16_t wheelBend, int16_t deadZone);
