#include "heldNotes.h"

void HeldNotes::add(uint8_t note) {
  remove(note);                             // never list the same key twice
  if (count == MAX_HELD_NOTES) {
    remove(notes[0]);                       // full: drop the oldest
  }
  notes[count] = note;
  count++;
}

void HeldNotes::remove(uint8_t note) {
  for (int16_t i = 0; i < count; i++) {
    if (notes[i] == note) {
      // Shift every later key one slot down, over the removed one.
      for (int16_t j = i; j < count - 1; j++) {
        notes[j] = notes[j + 1];
      }
      count--;
      return;
    }
  }
}

bool HeldNotes::any() const {
  return count > 0;
}

uint8_t HeldNotes::sounding(NotePriority priority) const {
  if (priority == NotePriority::Low) {
    uint8_t lowest = notes[0];
    for (int16_t i = 1; i < count; i++) {
      if (notes[i] < lowest) {
        lowest = notes[i];
      }
    }
    return lowest;
  }

  if (priority == NotePriority::High) {
    uint8_t highest = notes[0];
    for (int16_t i = 1; i < count; i++) {
      if (notes[i] > highest) {
        highest = notes[i];
      }
    }
    return highest;
  }

  return notes[count - 1];                  // Last: the newest key
}
