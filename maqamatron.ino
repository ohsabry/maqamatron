// Maqamatron
// Knob A0 picks the tonic, knob A1 picks the maqam.
// Shield switch: PROG to upload, RUN to play.

#include <MIDI.h>
#include <LiquidCrystal.h>

MIDI_CREATE_DEFAULT_INSTANCE();              // MIDI on hardware serial (D0/D1)
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);     // RS, E, D4, D5, D6, D7

const int BUTTON_PIN = 4;            // bypass button: reads LOW while pressed

const long BEND_RANGE_CENTS = 200;   // MicroBrute bend range: +/-2 semitones (verified by ear)
const int  BEND_MIN = -8192;         // MIDI library bends are centered on 0
const int  BEND_MAX = 8191;

const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F",
                           "F#", "G", "G#", "A", "A#", "B"};

// ---- Maqam tables ----
// Cents offset for each key, counted in semitones above the tonic.
// A half-flat (E-) is the natural key bent down 50 cents. Tweak any number by ear.
// The order of this list is the order on the maqam knob.
struct Maqam {
  const char* name;
  int cents[12];
};

const Maqam maqams[] = {
  //                  0    1    2    3    4    5    6    7    8    9   10   11
  { "Bayati",       {  0,   0, -50,   0,   0,   0,   0,   0,   0,   0,   0,   0 } },  // D E- F G A Bb C
                                                                                         // also covers Saba (Gb) and
                                                                                         // Bayati Shuri (Ab, B): same bends
  { "Sikah",       {-50,   0,   0,   0,   0,   0,   0, -50,   0,   0,   0,   0 } },  // E- F G A B- C D
  { "Huzam",        {-50,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 } },  // E- F G Ab B C D
  { "Rast",         {  0,   0,   0,   0, -50,   0,   0,   0,   0,   0,   0, -50 } },  // C D E- F G A B- (Bb desc)
  { "Husseini",     {  0,   0, -50,   0,   0,   0,   0,   0,   0, -50,   0,   0 } },  // D E- F G A B- C
  { "Suznak",       {  0,   0,   0,   0, -50,   0,   0,   0,   0,   0,   0,   0 } },  // C D E- F G Ab B
};

const int maqamCount = sizeof(maqams) / sizeof(maqams[0]);

// Current settings, set by the knobs. -1 means "not read yet",
// so the first knob reading always counts as a change and draws the LCD.
int tonic = -1;
int maqam = -1;

// Turn cents into a pitch bend value (0 = no bend).
int centsToBend(int cents) {
  long bend = (long)cents * 8192 / BEND_RANGE_CENTS;   // long: 50 * 8192 overflows an int
  if (bend < BEND_MIN) bend = BEND_MIN;
  if (bend > BEND_MAX) bend = BEND_MAX;
  return (int)bend;
}

// Show the maqam name and tonic on LCD line 1, e.g. "Husseini       D".
void showMaqam() {
  lcd.setCursor(0, 0);
  lcd.print("                ");            // wipe the old line
  lcd.setCursor(0, 0);
  lcd.print(maqams[maqam].name);
  lcd.setCursor(14, 0);                     // tonic in the last two columns
  lcd.print(noteNames[tonic]);
}

// Show the last note, its offset and the bend on LCD line 2, e.g. "E4 -50c  b-2048".
void showNote(byte note, int cents, int bend) {
  lcd.setCursor(0, 1);
  lcd.print("                ");            // wipe the old line
  lcd.setCursor(0, 1);
  lcd.print(noteNames[note % 12]);
  lcd.print(note / 12 - 1);                 // octave (MIDI note 60 = C4)
  lcd.print(" ");
  lcd.print(cents);
  lcd.print("c  b");
  lcd.print(bend);
}

// Read both knobs. Each knob's 0..1023 range is cut into equal slices.
// map(x, 0, 1024, ...) rather than 1023, so the top slice is as wide as the others.
void readKnobs() {
  int newTonic = map(analogRead(A0), 0, 1024, 0, 12);
  int newMaqam = map(analogRead(A1), 0, 1024, 0, maqamCount);

  if (newTonic != tonic || newMaqam != maqam) {   // only redraw on change
    tonic = newTonic;
    maqam = newMaqam;
    showMaqam();
  }
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  // Semitones above the tonic, any octave: % 12 drops the octaves.
  // The + 12 keeps it from going negative for notes below the tonic
  // (in C++, -1 % 12 is -1, not 11). Adding a whole octave never
  // changes the answer, it just keeps the array index valid.
  int step  = (note + 12 - tonic) % 12;
  int cents = maqams[maqam].cents[step];
  int bend  = centsToBend(cents);

  MIDI.sendPitchBend(bend, channel);        // bend first...
  MIDI.sendNoteOn(note, velocity, channel); // ...then the note

  showNote(note, cents, bend);
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  MIDI.sendNoteOff(note, velocity, channel);
}

void setup() {
  lcd.begin(16, 2);
  pinMode(BUTTON_PIN, INPUT_PULLUP);        // internal resistor holds the pin HIGH until pressed
  readKnobs();                              // know the tonic and maqam before any note arrives

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.begin(MIDI_CHANNEL_OMNI);            // listen on all channels
  MIDI.turnThruOff();                       // we forward messages ourselves
}

void loop() {
  MIDI.read();                              // calls our handlers when a message arrives
  readKnobs();

  // TEMPORARY button test: show the raw pin reading (1 = HIGH, 0 = LOW).
  lcd.setCursor(11, 0);
  lcd.print(digitalRead(BUTTON_PIN));
}
