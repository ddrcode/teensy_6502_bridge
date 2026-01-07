FQBN ?= teensy:avr:teensy41
PORT ?= /dev/ttyACM0
TARGET ?= target

TEST_SRC ?= tests/*.cpp
TEST_SRC += tests/mocks/*.cpp
TEST_SRC += src/protocol.cpp
TEST_SRC += src/pins.cpp
TEST_SRC += src/cpu.cpp
TEST_SRC += src/io.cpp

build:
	arduino-cli compile -b $(FQBN) --build-path $(TARGET) .

debug:
	arduino-cli compile -b $(FQBN) --build-property "build.extra_flags=\"-DDEBUG_TEENSY_COM_BRIDGE\"" .

upload:
	# arduino-cli upload -b $(FQBN) --port $(PORT) --build-path $(TARGET) .
	teensy-loader-cli --mcu TEENSY41 -w -v $(TARGET)/teensy_6502_bridge.ino.hex

test:
	$(CXX) -std=c++17 -o tests/tests -DRUNNING_TESTS $(TEST_SRC)
	./tests/tests

