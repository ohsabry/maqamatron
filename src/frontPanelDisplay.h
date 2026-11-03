// FrontPanelDisplay: the 16x2 LCD and the maqam LED.
//
// showMaqam() and showNote() only record what to show. update() redraws
// at most every 100 ms, because a full LCD redraw takes about 7 ms and
// would hold up MIDI.

#pragma once

#include <stdint.h>
#include <LiquidCrystal.h>

#include "display.h"

class FrontPanelDisplay : public Display {
public:
  FrontPanelDisplay();

  void begin();     // call once in setup()
  void update();    // call every loop()

  void showMaqam(const char* maqamName, int16_t tonic, bool bypass) override;
  void showNote(uint8_t note, int16_t cents, int16_t bend) override;

private:
  void printSpaces(int16_t count);
  void drawMaqamLine();
  void drawNoteLine();
  void showMaqamLed();

  LiquidCrystal lcd;

  const char* shownMaqamName = "";
  int16_t shownTonic  = 0;
  bool    shownBypass = false;

  bool    noteHasBeenPlayed = false;   // line 2 stays blank until then
  uint8_t shownNote  = 0;
  int16_t shownCents = 0;
  int16_t shownBend  = 0;

  bool dirty = false;
  unsigned long lastUpdateTime = 0;
};
