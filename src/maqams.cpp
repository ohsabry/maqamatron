#include "maqams.h"

const char* noteNames[12] = {"C", "C#", "D", "D#", "E", "F",
                             "F#", "G", "G#", "A", "A#", "B"};

// Half-flats are -50 cents to start with. Real intonation varies by maqam
// and by player (roughly -40 to -60), so tune these by ear.
// Scales are shown on their usual tonic; "-" marks a half-flat.
// Bayati also covers Saba and Bayati Shuri, which bend the same keys.
// Names must fit in 14 characters, the LCD space left of the tonic.
const Maqam maqams[] = {
  //                  0    1    2    3    4    5    6    7    8    9   10   11
  { "Bayati",       {  0,   0, -50,   0,   0,   0,   0,   0,   0,   0,   0,   0 } },  // D E- F G A Bb C
  { "Sikah",        {-50,   0,   0,   0,   0,   0,   0, -50,   0,   0,   0,   0 } },  // E- F G A B- C D
  { "Huzam",        {-50,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 } },  // E- F G Ab B C D
  { "Rast",         {  0,   0,   0,   0, -50,   0,   0,   0,   0,   0,   0, -50 } },  // C D E- F G A B- (Bb descending)
  { "Husseini",     {  0,   0, -50,   0,   0,   0,   0,   0,   0, -50,   0,   0 } },  // D E- F G A B- C
  { "Suznak",       {  0,   0,   0,   0, -50,   0,   0,   0,   0,   0,   0,   0 } },  // C D E- F G Ab B
};

const int16_t maqamCount = sizeof(maqams) / sizeof(maqams[0]);

int16_t maqamCentsForNote(uint8_t note, int16_t tonic, int16_t maqamIndex) {
  // Semitones above the tonic, in any octave. The + 12 keeps it from going
  // negative for notes below the tonic: in C++, -1 % 12 is -1, not 11.
  int16_t step = (note + 12 - tonic) % 12;
  return maqams[maqamIndex].cents[step];
}
