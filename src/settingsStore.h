// Settings saved on the box, as bytes for the EEPROM. Pure code, so the
// sketch does the actual EEPROM reads and writes.
//
// Layout: magic, layout version, bend range in semitones, note priority,
// checksum. Only the settings that must match the synth are stored; the
// rest always come from settings.h.

#pragma once

#include <stdint.h>

#include "engine.h"

const uint8_t SETTINGS_BYTES = 5;
const int16_t MAX_BEND_RANGE_SEMITONES = 12;

void encodeSettings(const EngineSettings& settings, uint8_t bytes[SETTINGS_BYTES]);

// Returns the defaults if the bytes aren't valid saved settings: a new
// chip's EEPROM (all 0xFF), another sketch's data, a write cut short by
// pulling the power, or a value out of range.
EngineSettings decodeSettings(const uint8_t bytes[SETTINGS_BYTES], const EngineSettings& defaults);
