# Maqamatron

A MIDI box for playing Arabic (maqam) music on a mono synth. It sits between a MIDI keyboard or controller and the synth: pick a maqam and a tonic, play normally, and it retunes each note to the maqam's quarter tones using pitch bend.

It's the same idea as the sikah buttons on the "oriental" keyboards you find all over the Arab world, but it lets you play maqamat on the Western-tuned mono synthesizer you already have.

## Hardware

- Arduino Uno or SparkFun RedBoard
- SparkFun MIDI Shield
- 16x2 LCD

| Part                 | Pin                  |
| -------------------- | -------------------- |
| LCD (RS, E, DB4–DB7) | 13, 12, 11, 10, 9, 8 |
| Tonic knob           | A0 (on the shield)   |
| Maqam knob           | A1 (on the shield)   |
| Bypass button        | D4 (on the shield)   |
| Maqam LED            | D6 (on the shield)   |

## Using it

- The A0 knob picks the tonic, the A1 knob the maqam.
- The button toggles bypass. The LED is lit while the maqam is active.
- The LCD shows the maqam and tonic, and the last note with its bend.
- At power-on it shows the firmware version for a second.

## Synth setup

Pitch bend moves every note on a MIDI channel at once, so this only works with a mono synth. Two settings must match the synth:

- Pitch bend range: 1–12 semitones, default ±2
- Note priority: last, low or high, default last

To change them, hold the button while powering on until the settings screen appears. The A0 knob picks the setting, the A1 knob changes it, and the button saves. The defaults are in [`src/settings.h`](src/settings.h).

## Maqamat

Bayati, Sikah, Huzam, Rast, Husseini, Suznak.

Bayati also covers Saba and Bayati Shuri, which bend the same notes. Maqamat without quarter tones (Hijaz, Kurd, Nahawand, Ajam) play in tune in bypass.

Half-flats are a strict 50 cents. Real intonation varies by maqam, region, and player, so tune them by ear, or add your own maqamat, in [`src/maqams.cpp`](src/maqams.cpp). The maqam knob adjusts to however many there are.

## Build

Needs [arduino-cli](https://arduino.github.io/arduino-cli/), the [MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library) by Francois Best (5.x), and LiquidCrystal, which comes with the Arduino IDE. The Makefile looks for the IDE's libraries in the macOS location; change `IDE_LIBS` on other systems.

```
arduino-cli core install arduino:avr
arduino-cli lib install "MIDI Library"

make          # compile
make upload   # shield switch on PROG to upload, RUN to play
make test     # run tests on the computer
```

## License

MIT
