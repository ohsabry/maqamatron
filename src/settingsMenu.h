// The settings menu: hold the button at power-on to open it.
//
// The tonic knob picks the setting, the maqam knob its value, and the
// button leaves the menu. The caller saves the result.

#pragma once

#include "engine.h"
#include "frontPanelDisplay.h"

void runSettingsMenu(FrontPanelDisplay& display, EngineSettings& settings);
