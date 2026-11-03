// Pitch bend math.

#pragma once

#include <stdint.h>

// MIDI pitch bend is 14 bits; the MIDI library centres it on 0.
const int16_t BEND_MIN = -8192;
const int16_t BEND_MAX = 8191;

// rangeCents: the synth's full bend range, e.g. 200 for +/-2 semitones.
int16_t centsToBend(int16_t cents, int16_t rangeCents);

// Clamped to the bend range, so at the edges the maqam offset is lost
// (e.g. a half-flat with the wheel fully down).
int16_t combineBend(int16_t wheelBend, int16_t noteBend);

// Values within deadZone of 0 become 0.
int16_t applyWheelDeadZone(int16_t wheelBend, int16_t deadZone);
