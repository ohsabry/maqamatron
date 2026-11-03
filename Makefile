# Maqamatron build helpers (needs arduino-cli: brew install arduino-cli)
#
#   make            compile (same as make compile)
#   make upload     compile and upload: shield switch on PROG first!
#   make ports      list connected boards, to find the PORT
#   make test       run the tests on this computer (no board needed)
#   make clean      delete build output
#
# Override the port if it's not found automatically:
#   make upload PORT=/dev/cu.usbserial-XXXX

FQBN      := arduino:avr:uno
BUILD_DIR := build
# LiquidCrystal ships with the Arduino IDE, in its own libraries folder.
IDE_LIBS  := $(HOME)/Library/Arduino15/libraries
# The RedBoard shows up as a USB serial port; take the first one found.
PORT      ?= $(firstword $(wildcard /dev/cu.usbserial-* /dev/cu.usbmodem*))

CLI_FLAGS := --fqbn $(FQBN) --libraries $(IDE_LIBS) --build-path $(BUILD_DIR)

.PHONY: compile upload ports test clean

compile:
	arduino-cli compile $(CLI_FLAGS) --warnings all .

upload: compile
	@test -n "$(PORT)" || { echo "No board found. Plug it in, or pass PORT=..."; exit 1; }
	@echo "Uploading to $(PORT). Shield switch on PROG? Switch to RUN afterwards."
	arduino-cli upload --fqbn $(FQBN) --input-dir $(BUILD_DIR) --port $(PORT) .

ports:
	arduino-cli board list

# Tests build with the computer's own compiler, from only the modules that
# don't touch hardware. The sanitizers fail the run on out-of-bounds memory
# access and integer overflow, which would otherwise pass silently.
TEST_SRCS := test/tests.cpp src/bend.cpp src/maqams.cpp src/heldNotes.cpp src/engine.cpp
TEST_BIN  := $(BUILD_DIR)/host/tests

test:
	@mkdir -p $(dir $(TEST_BIN))
	$(CXX) -std=c++17 -Wall -Wextra -fsanitize=address,undefined -fno-sanitize-recover=all -Isrc $(TEST_SRCS) -o $(TEST_BIN)
	$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)
