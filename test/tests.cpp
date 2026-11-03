// Host tests: run on the computer with `make test`, no board needed.
//
// They cover the logic that's easy to get subtly wrong: bend math at the
// edges of its range, the maqam lookup across octaves, the held-notes
// fallback, and what the engine sends, in what order.
//
// The modules use fixed-width types (int16_t etc.), so values are stored
// at the same sizes as on the Uno. Arithmetic isn't: the computer's int is
// 32 bits and the Uno's is 16, so an overflow partway through a calculation
// happens only on the board. Those need checking by hand.

#include <cstdio>
#include <string>
#include <vector>

#include "bend.h"
#include "maqams.h"
#include "heldNotes.h"
#include "engine.h"

static int failures = 0;

#define CHECK(condition) check((condition), #condition, __LINE__)

static void check(bool ok, const char* what, int line) {
  if (!ok) {
    std::printf("  FAILED line %d: %s\n", line, what);
    failures++;
  }
}

// Fakes that record what the engine asked for.

class FakeSynth : public Synth {
public:
  std::vector<std::string> sent;

  void noteOn(uint8_t channel, uint8_t note, uint8_t velocity) override {
    sent.push_back("noteOn ch" + std::to_string(channel) + " " + std::to_string(note) + " v" + std::to_string(velocity));
  }
  void noteOff(uint8_t channel, uint8_t note, uint8_t) override {
    sent.push_back("noteOff ch" + std::to_string(channel) + " " + std::to_string(note));
  }
  void bend(uint8_t channel, int16_t bend) override {
    sent.push_back("bend ch" + std::to_string(channel) + " " + std::to_string(bend));
  }
  void passThrough(bool on) override {
    sent.push_back(on ? "passThrough on" : "passThrough off");
  }
};

class FakeDisplay : public Display {
public:
  std::string maqamName;
  int16_t tonic = -1;
  bool bypass = false;
  int16_t cents = 0;

  void showMaqam(const char* name, int16_t t, bool b) override {
    maqamName = name;
    tonic = t;
    bypass = b;
  }
  void showNote(uint8_t, int16_t c, int16_t) override {
    cents = c;
  }
};

const uint8_t C4 = 60, D4 = 62, E4 = 64, G4 = 67;
const int16_t TONIC_C = 0;
const int16_t BAYATI = 0, RAST = 3;
const int16_t HALF_FLAT_BEND = -2048;     // -50 cents at +/-200

static const EngineSettings SETTINGS = { 200, 64, NotePriority::Last };


static void testBendMath() {
  CHECK(centsToBend(-50, 200) == HALF_FLAT_BEND);
  CHECK(centsToBend(200, 200) == BEND_MAX);         // +8192 doesn't fit: clamped to 8191
  CHECK(combineBend(8000, 2048) == BEND_MAX);       // wheel near the top plus an offset
  CHECK(combineBend(-8000, -2048) == BEND_MIN);
  CHECK(applyWheelDeadZone(63, 64) == 0);
  CHECK(applyWheelDeadZone(-63, 64) == 0);
  CHECK(applyWheelDeadZone(64, 64) == 64);
}

static void testMaqamLookup() {
  // Bayati on C: D is half-flat in every octave, C and E aren't.
  CHECK(maqamCentsForNote(D4, TONIC_C, BAYATI) == -50);
  CHECK(maqamCentsForNote(D4 + 24, TONIC_C, BAYATI) == -50);
  CHECK(maqamCentsForNote(C4, TONIC_C, BAYATI) == 0);
  CHECK(maqamCentsForNote(E4, TONIC_C, BAYATI) == 0);

  // A note whose pitch class is below the tonic's (the + 12 in the lookup).
  // Rast on D: C# is the half-flat 7th. Note 1 is the lowest C# MIDI has.
  const int16_t TONIC_D = 2;
  CHECK(maqamCentsForNote(1, TONIC_D, RAST) == -50);
  CHECK(maqamCentsForNote(C4 + 1, TONIC_D, RAST) == -50);
}

static void testHeldNotesFallback() {
  HeldNotes held;
  held.add(C4);
  held.add(D4);
  held.add(E4);
  held.remove(E4);
  CHECK(held.sounding(NotePriority::Last) == D4);   // falls back to the previous key
  held.remove(C4);
  CHECK(held.sounding(NotePriority::Last) == D4);   // an older key let go: no change
  held.remove(D4);
  CHECK(!held.any());
}

static void testHeldNotesSameKeyTwice() {
  // A key pressed again (e.g. a missed note-off) is listed once, as newest.
  HeldNotes held;
  held.add(C4);
  held.add(D4);
  held.add(C4);
  CHECK(held.sounding(NotePriority::Last) == C4);
  held.remove(C4);
  CHECK(held.sounding(NotePriority::Last) == D4);
}

static void testHeldNotesFull() {
  // One more key than fits: the oldest is dropped, nothing overflows.
  HeldNotes held;
  for (uint8_t note = 0; note <= MAX_HELD_NOTES; note++) {
    held.add(note);
  }
  for (uint8_t note = 1; note <= MAX_HELD_NOTES; note++) {
    held.remove(note);
  }
  CHECK(!held.any());                               // note 0 was the one dropped
}

static void testHeldNotesPriority() {
  HeldNotes held;
  held.add(E4);
  held.add(C4);
  held.add(G4);
  CHECK(held.sounding(NotePriority::Low) == C4);
  CHECK(held.sounding(NotePriority::High) == G4);
  CHECK(held.sounding(NotePriority::Last) == G4);
}

static void testEngineBendsBeforeNote() {
  FakeSynth synth;
  FakeDisplay display;
  Engine engine(synth, display, SETTINGS);
  engine.setMaqam(TONIC_C, BAYATI);

  engine.noteOn(1, D4, 100);
  // The bend has to arrive first, or the note starts out of tune.
  CHECK((synth.sent == std::vector<std::string>{ "bend ch1 -2048", "noteOn ch1 62 v100" }));
  CHECK(display.cents == -50);
}

static void testEngineWheelAddsToOffset() {
  FakeSynth synth;
  FakeDisplay display;
  Engine engine(synth, display, SETTINGS);
  engine.setMaqam(TONIC_C, BAYATI);
  engine.noteOn(1, D4, 100);
  synth.sent.clear();

  engine.wheel(1, 1000);
  CHECK((synth.sent == std::vector<std::string>{ "bend ch1 -1048" }));
}

static void testEngineFallbackRetunes() {
  FakeSynth synth;
  FakeDisplay display;
  Engine engine(synth, display, SETTINGS);
  engine.setMaqam(TONIC_C, BAYATI);
  engine.noteOn(1, C4, 100);
  engine.noteOn(1, D4, 100);
  synth.sent.clear();

  // Let go of the half-flat D: the synth falls back to C, which is in tune.
  engine.noteOff(1, D4, 0);
  CHECK((synth.sent == std::vector<std::string>{ "noteOff ch1 62", "bend ch1 0" }));
}

static void testEngineKnobRetunesHeldNote() {
  FakeSynth synth;
  FakeDisplay display;
  Engine engine(synth, display, SETTINGS);
  engine.setMaqam(TONIC_C, BAYATI);
  engine.noteOn(1, E4, 100);                        // E is natural in Bayati on C
  synth.sent.clear();

  engine.setMaqam(TONIC_C, RAST);                   // ...and half-flat in Rast on C
  CHECK((synth.sent == std::vector<std::string>{ "bend ch1 -2048" }));
  CHECK(display.maqamName == "Rast");
}

static void testEngineBypass() {
  FakeSynth synth;
  FakeDisplay display;
  Engine engine(synth, display, SETTINGS);
  engine.setMaqam(TONIC_C, BAYATI);
  engine.wheel(1, 1000);
  engine.noteOn(1, D4, 100);
  synth.sent.clear();

  // Entering bypass drops the maqam offset but keeps the wheel.
  engine.toggleBypass();
  CHECK((synth.sent == std::vector<std::string>{ "bend ch1 1000", "passThrough on" }));
  CHECK(display.bypass);

  // In bypass the engine sends nothing (thru forwards it), but keeps track:
  synth.sent.clear();
  engine.noteOff(1, D4, 0);
  engine.noteOn(2, D4 + 12, 100);
  engine.wheel(2, 0);
  CHECK(synth.sent.empty());

  // ...so leaving bypass retunes the key held now, on its channel.
  engine.toggleBypass();
  CHECK((synth.sent == std::vector<std::string>{ "passThrough off", "bend ch2 -2048" }));
  CHECK(!display.bypass);
}


int main() {
  testBendMath();
  testMaqamLookup();
  testHeldNotesFallback();
  testHeldNotesSameKeyTwice();
  testHeldNotesFull();
  testHeldNotesPriority();
  testEngineBendsBeforeNote();
  testEngineWheelAddsToOffset();
  testEngineFallbackRetunes();
  testEngineKnobRetunesHeldNote();
  testEngineBypass();

  if (failures > 0) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("All tests passed\n");
  return 0;
}
