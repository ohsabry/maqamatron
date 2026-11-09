#include "settingsStore.h"

static const uint8_t MAGIC  = 0x4D;   // 'M'
static const uint8_t LAYOUT = 1;      // bump when the layout changes

static uint8_t checksum(const uint8_t bytes[SETTINGS_BYTES]) {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < SETTINGS_BYTES - 1; i++) {
    sum += bytes[i];
  }
  return sum;
}

void encodeSettings(const EngineSettings& settings, uint8_t bytes[SETTINGS_BYTES]) {
  bytes[0] = MAGIC;
  bytes[1] = LAYOUT;
  bytes[2] = settings.bendRangeCents / 100;
  bytes[3] = (uint8_t)settings.notePriority;
  bytes[4] = checksum(bytes);
}

EngineSettings decodeSettings(const uint8_t bytes[SETTINGS_BYTES], const EngineSettings& defaults) {
  uint8_t semitones = bytes[2];
  uint8_t priority  = bytes[3];

  if (bytes[0] != MAGIC || bytes[1] != LAYOUT || bytes[4] != checksum(bytes)) {
    return defaults;
  }
  // A bend range of 0 would divide by zero in centsToBend().
  if (semitones < 1 || semitones > MAX_BEND_RANGE_SEMITONES || priority > (uint8_t)NotePriority::High) {
    return defaults;
  }

  EngineSettings settings = defaults;
  settings.bendRangeCents = semitones * 100;
  settings.notePriority   = (NotePriority)priority;
  return settings;
}
