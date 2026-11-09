// The Makefile sets FIRMWARE_VERSION from git (see VERSION there).
// Builds from the Arduino IDE don't, so they show "dev".

#pragma once

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "dev"
#endif
