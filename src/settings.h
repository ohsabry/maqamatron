// Defaults for the settings that match the synth and the player.
// Bend range and note priority can also be changed on the box (hold the
// button at power-on); saved values replace these defaults.

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
