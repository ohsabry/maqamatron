#include "bend.h"

#include <stdlib.h>   // abs()

int centsToBend(int cents, int rangeCents) {
  long bend = (long)cents * 8192 / rangeCents;   // long: 50 * 8192 overflows a 16-bit int
  if (bend < BEND_MIN) bend = BEND_MIN;
  if (bend > BEND_MAX) bend = BEND_MAX;
  return (int)bend;
}

int combineBend(int wheelBend, int noteBend) {
  int total = wheelBend + noteBend;              // at most about +/-16000, fits in an int
  if (total < BEND_MIN) total = BEND_MIN;
  if (total > BEND_MAX) total = BEND_MAX;
  return total;
}

int applyWheelDeadZone(int wheelBend, int deadZone) {
  if (abs(wheelBend) < deadZone) {
    return 0;
  }
  return wheelBend;
}
