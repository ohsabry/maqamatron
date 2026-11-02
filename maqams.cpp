#include "maqams.h"

const char* noteNames[12] = {"C", "C#", "D", "D#", "E", "F",
                             "F#", "G", "G#", "A", "A#", "B"};

// Tweak any number by ear to taste.
const Maqam maqams[] = {
  //                  0    1    2    3    4    5    6    7    8    9   10   11
  { "Bayati",       {  0,   0, -50,   0,   0,   0,   0,   0,   0,   0,   0,   0 } },  // D E- F G A Bb C
                                                                                         // also covers Saba (Gb) and
                                                                                         // Bayati Shuri (Ab, B): same bends
  { "Sikah",        {-50,   0,   0,   0,   0,   0,   0, -50,   0,   0,   0,   0 } },  // E- F G A B- C D
  { "Huzam",        {-50,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 } },  // E- F G Ab B C D
  { "Rast",         {  0,   0,   0,   0, -50,   0,   0,   0,   0,   0,   0, -50 } },  // C D E- F G A B- (Bb desc)
  { "Husseini",     {  0,   0, -50,   0,   0,   0,   0,   0,   0, -50,   0,   0 } },  // D E- F G A B- C
  { "Suznak",       {  0,   0,   0,   0, -50,   0,   0,   0,   0,   0,   0,   0 } },  // C D E- F G Ab B
};

const int maqamCount = sizeof(maqams) / sizeof(maqams[0]);

int maqamCentsForNote(uint8_t note, int tonic, int maqamIndex) {
  // Semitones above the tonic, any octave: % 12 drops the octaves.
  // The + 12 keeps it from going negative for notes below the tonic
  // (in C++, -1 % 12 is -1, not 11). Adding a whole octave never
  // changes the answer, it just keeps the array index valid.
  int step = (note + 12 - tonic) % 12;
  return maqams[maqamIndex].cents[step];
}
