#include "settingsMenu.h"

#include <Arduino.h>
#include <stdio.h>

#include "controls.h"
#include "settingsStore.h"

enum Setting { BEND_RANGE, NOTE_PRIORITY_SETTING, SETTING_COUNT };

static const char* priorityNames[] = {"Last", "Low", "High"};

static int16_t valueCount(int16_t setting) {
  return setting == BEND_RANGE ? MAX_BEND_RANGE_SEMITONES : 3;
}

static int16_t currentValue(const EngineSettings& settings, int16_t setting) {
  if (setting == BEND_RANGE) {
    return settings.bendRangeCents / 100 - 1;
  }
  return (int16_t)settings.notePriority;
}

static void setValue(EngineSettings& settings, int16_t setting, int16_t value) {
  if (setting == BEND_RANGE) {
    settings.bendRangeCents = (value + 1) * 100;
  } else {
    settings.notePriority = (NotePriority)value;
  }
}

static void draw(FrontPanelDisplay& display, const EngineSettings& settings, int16_t setting) {
  if (setting == BEND_RANGE) {
    char value[17];
    int16_t semitones = settings.bendRangeCents / 100;
    snprintf(value, sizeof(value), "%d semitone%s", semitones, semitones == 1 ? "" : "s");
    display.showSetting("Bend range", value);
  } else {
    display.showSetting("Note priority", priorityNames[(int16_t)settings.notePriority]);
  }
}

void runSettingsMenu(FrontPanelDisplay& display, EngineSettings& settings) {
  // The button is still down from power-on. Wait for it to come up and stop
  // bouncing, or the release would count as a press and close the menu.
  while (controlsBypassHeld()) {}
  delay(50);

  int16_t setting = -1;
  int16_t knobAtEntry = 0;
  bool knobMoved = false;

  while (true) {
    controlsUpdate();
    if (controlsBypassPressed()) {
      break;
    }

    int16_t newSetting = controlsTonicKnob(SETTING_COUNT);
    int16_t knob = controlsMaqamKnob(valueCount(newSetting));
    bool changed = false;

    // The maqam knob is wherever it was left, so it only takes over once
    // turned. Otherwise opening a setting would overwrite its value.
    if (newSetting != setting) {
      setting = newSetting;
      knobAtEntry = knob;
      knobMoved = false;
      changed = true;
    }
    if (knob != knobAtEntry) {
      knobMoved = true;
    }
    if (knobMoved && knob != currentValue(settings, setting)) {
      setValue(settings, setting, knob);
      changed = true;
    }

    if (changed) {
      draw(display, settings, setting);
    }
  }

  display.showSetting("Saved", "");
  delay(1000);
}
