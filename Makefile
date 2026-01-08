FQBN ?= teensy:avr:teensy41
PORT ?=
TARGET ?= target
PROGRAM ?= examples/test.p
PROGRAM_ABS := $(abspath $(PROGRAM))

CPP_EXAMPLE := examples/cpp/example.out
CPP_EXAMPLE_SRCS := $(wildcard examples/cpp/*.cpp)

.PHONY: build debug upload test run-cpp build-cpp-example run-rust detect-port

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

$(CPP_EXAMPLE): $(CPP_EXAMPLE_SRCS)
	$(CXX) -std=c++17 -O2 -o $@ $^

build-cpp-example: $(CPP_EXAMPLE)

run-cpp: build-cpp-example
	@if [ -z "$(PORT)" ]; then \
	  echo "Error: PORT is not set. Use PORT=/dev/ttyACM0 make run-cpp"; \
	  exit 1; \
	fi
	$(CPP_EXAMPLE) --port $(PORT) --program $(PROGRAM_ABS)

run-rust:
	@if [ -z "$(PORT)" ]; then \
	  echo "Error: PORT is not set. Use PORT=/dev/ttyACM0 make run-rust"; \
	  exit 1; \
	fi
	cd examples/rust && cargo run -- --port $(PORT) --program $(PROGRAM_ABS)

detect-port:
	@arduino-cli board list

