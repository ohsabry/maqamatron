// What the engine shows the player. FrontPanelDisplay is the real one.
// The engine calls these between MIDI messages, so they must return quickly.

#pragma once

#include <stdint.h>

class Display {
public:
  virtual void showMaqam(const char* maqamName, int16_t tonic, bool bypass) = 0;

  // A note that was just tuned, its offset and the bend sent.
  virtual void showNote(uint8_t note, int16_t cents, int16_t bend) = 0;
};
