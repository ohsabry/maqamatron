// Settings to match the synth and the player. maqamatron.ino passes them
// to the engine.

#pragma once

#include <stdint.h>

#include "heldNotes.h"

// Must match the synth's pitch bend range. 200 = +/-2 semitones.
const int16_t BEND_RANGE_CENTS = 200;

// Wheel readings this close to centre count as centred, so a wheel resting
// slightly off doesn't detune every note. 64 is about 1.5 cents at +/-2
// semitones.
const int16_t WHEEL_DEAD_ZONE = 64;

// Must match the synth's note priority.
const NotePriority NOTE_PRIORITY = NotePriority::Last;
