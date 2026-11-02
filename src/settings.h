// Settings you might change to suit your synth or your playing.
// Only maqamatron.ino includes this; modules get these values passed in.

#pragma once

#include <stdint.h>

#include "heldNotes.h"   // NotePriority

// How far a full pitch bend goes on the synth, in cents.
// Must match the synth's own bend range setting: 200 = +/-2 semitones.
const int16_t BEND_RANGE_CENTS = 200;

// A wheel resting a hair off centre counts as centred.
// 64 of 8192 is about 1.5 cents at +/-2 semitones, too small to hear as a step.
const int16_t WHEEL_DEAD_ZONE = 64;

// Which held key the synth plays when several are held.
// Must match the synth's own note priority setting.
const NotePriority NOTE_PRIORITY = NotePriority::Last;
