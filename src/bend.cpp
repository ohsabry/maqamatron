#include "bend.h"

#include <stdlib.h>   // abs()

int16_t centsToBend(int16_t cents, int16_t rangeCents) {
  int32_t bend = (int32_t)cents * 8192 / rangeCents;   // 32 bits: 50 * 8192 overflows 16
  if (bend < BEND_MIN) bend = BEND_MIN;
  if (bend > BEND_MAX) bend = BEND_MAX;
  return (int16_t)bend;
}

int16_t combineBend(int16_t wheelBend, int16_t noteBend) {
  int16_t total = wheelBend + noteBend;          // at most about +/-16000, fits in 16 bits
  if (total < BEND_MIN) total = BEND_MIN;
  if (total > BEND_MAX) total = BEND_MAX;
  return total;
}

int16_t applyWheelDeadZone(int16_t wheelBend, int16_t deadZone) {
  if (abs(wheelBend) < deadZone) {
    return 0;
  }
  return wheelBend;
}
