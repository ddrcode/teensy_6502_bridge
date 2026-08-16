# C++ development guide

This guide covers the two C++ parts of the project: the Teensy firmware (the bridge itself) and
the C++ host runner in [examples/cpp](../examples/cpp/). For the wire format between them, see the
[protocol specification](./protocol.md).

## Firmware

### Source layout

| File | Responsibility |
| ---- | -------------- |
| `teensy_6502_bridge.ino`  | Entry point: `setup()`/`loop()` and the main request-response loop (`loop_prod`); selects the firmware mode at compile time |
| `configuration.h`         | User configuration: the `PINS_MAP` pin assignment, debug/diagnostics mode switches, debug clock speed |
| `src/pins.{hpp,cpp}`      | GPIO layer: pin-id mapping, reading/writing individual pins, (de)serializing all 40 pins to/from the 5-byte payload |
| `src/cpu.{hpp,cpp}`       | CPU control: pin directions, the reset sequence, and per-cycle data-bus direction handling (drive gated on `RW && PHI2`) |
| `src/io.{hpp,cpp}`        | Serial I/O: non-blocking message assembly (`try_read_msg`) and error replies (`send_error`) |
| `src/protocol.{hpp,cpp}`  | Message framing: types, sizes, checksums, buffer conversions - pure logic, no hardware access |
| `src/debug.{hpp,cpp}`     | Standalone debug mode (see below) |
| `src/diagnostics.{hpp,cpp}` | On-board diagnostics mode |
| `src/hardware.hpp`        | Includes the real Arduino/Teensy headers, or the test mocks when `RUNNING_TESTS` is defined |

A useful rule of thumb: `protocol.cpp` knows nothing about hardware, `pins.cpp`/`cpu.cpp` know
nothing about messages, and `io.cpp` plus the `.ino` glue the two worlds together.

### Building and flashing

```bash
make build     # compile with arduino-cli into ./target
make upload    # flash with teensy-loader-cli
```

Prerequisites and alternatives (Arduino IDE, Nix shell) are described in the
[main README](../README.md). On macOS `make upload` doesn't work - see the
[macOS guide](./macos.md) for the working alternatives.

### Firmware modes

The mode is selected at compile time in `configuration.h` (or via compiler flags):

- **Production** (default) - the serial-controlled bridge loop described in the
  [protocol specification](./protocol.md).
- **Debug** (`#define DEBUG_TEENSY_BRIDGE`, or `make debug`) - the bridge clocks the CPU by itself
  (one half-cycle every `CYCLE_DURATION` milliseconds) and prints the address/data/status pins to
  the serial port. No host application is needed - this mode is handy for verifying your wiring
  with nothing but a serial monitor.
- **Diagnostics** (`#define ENABLE_DIAGNOSTICS`) - on-board diagnostics firmware.

### Tests

The firmware logic is covered by host-side unit and integration tests - no hardware required:

```bash
make test
```

This compiles everything in `tests/` together with the `src/` modules, with `RUNNING_TESTS`
defined so that `src/hardware.hpp` pulls in the mocks from `tests/mocks/` instead of the Arduino
headers. The mocked `Serial` supports scripted input (`_set_read_buff`), `peek`/`available`, and
captures everything written to it (`_get_out_buff`), so both directions of the protocol can be
asserted on.

To add tests: create a `tests/test_<area>.cpp` file (the Makefile picks up `tests/*.cpp`
automatically), give it an `<area>_tests_all()` entry function, and register that function in
`tests/tests.hpp` and `tests/tests.cpp`.

## Host runner (examples/cpp)

The C++ runner executes a 6502 binary on the real CPU while emulating 64 kB of RAM on the host.

### Source layout

| File | Responsibility |
| ---- | -------------- |
| `main.cpp`          | CLI parsing, serial-port configuration (raw 8N1, DTR/RTS), wiring everything together |
| `runner.{cpp,hpp}`  | The clock loop: reset sequence, half-cycle stepping, memory reads/writes, logging, BRK detection |
| `pins.{cpp,hpp}`    | `W65C02Pins` - a decoded view of the 40 pins, convertible to/from the 5-byte payload |
| `pin_utils.{cpp,hpp}` | Low-level helpers for picking single pins and address/data fields out of a payload |
| `memory.{cpp,hpp}`  | 64 kB RAM emulation and program loading |
| `configuration.hpp` | Knobs: program/interrupt addresses, per-half-cycle delay, BRK/max-cycles stop conditions |

### Building and running

```bash
make build-cpp-example
PORT=/dev/ttyACM0 make run-cpp
# or manually:
./examples/cpp/example.out --port /dev/ttyACM0 --program examples/test.p [--halfcycles]
```

### Configuration knobs (`configuration.hpp`)

- `PROGRAM_ADDR` / `INTERRUPT_ADDR` - where the binary is loaded and where the IRQ/NMI vectors point.
- `CYCLE_DURATION` - extra delay per half-cycle in microseconds. `0` (the default) runs at full
  speed; the bridge's response is the synchronization point, so no delay is needed for correctness.
  Increase it to watch execution in slow motion (e.g. to observe board LEDs).
- `EXIT_ON_BRK` - stop when the CPU fetches a `BRK` opcode.
- `MAX_CYCLES` - hard stop after N cycles (`0` = no limit).

### Writing test programs

Any 6502 assembler producing a plain binary works. The examples use [ACME](https://github.com/meonwax/acme)
(available in the project's Nix shell):

```bash
acme -f plain -o program.p --cpu w65c02 program.asm
```

The runner loads the binary at `PROGRAM_ADDR` (`$0200` by default) and seeds the reset vector to
point at it, so the program needs no header. Ending with `BRK` makes the runner stop cleanly -
see [examples/test.asm](../examples/test.asm) for a complete, commented example.

## Formatting

The repository uses [treefmt](https://github.com/numtide/treefmt) (with astyle for C++) - run
`treefmt` before committing.
