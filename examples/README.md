# Example - program runner

This folder hosts the workstation-side programs (C++ and Rust) that toggle `PHI2`, emulate RAM, and exchange
pin states with the Teensy bridge over USB. Both implementations behave identically; choose the toolchain you
prefer.

## Requirements

- Teensy 4.1 running the bridge firmware from this repository
- A W65C02 wired to the Teensy (at minimum: address bus, data bus, `PHI2`, and `RW/`)
- A serial port path such as `/dev/ttyACM0`
- A 6502 binary (the demo uses `examples/test.p`)

## Quick start

Use the Makefile helpers to build and run either example with one command:

```bash
PORT=/dev/ttyACM0 make run-cpp
PORT=/dev/ttyACM0 make run-rust
PROGRAM=examples/custom.bin PORT=/dev/ttyACM0 make run-cpp
```

`PORT` selects the serial device while `PROGRAM` decides which binary is loaded into the emulated RAM. Append
`--halfcycles` to either command when you want to observe both halves of every cycle and include the `PHI1O/PHI2O`
signals in the log.

## Manual compilation

If you prefer to compile by hand, follow the steps below.

### C++ version

1. Discover the Teensy port: `arduino-cli board list`
2. Build: `g++ -std=c++17 -O2 -o example.out *.cpp`
3. Run: `./example.out --port /dev/ttyACM0 --program ../test.p [--halfcycles]`

### Rust version

1. Discover the Teensy port: `arduino-cli board list`
2. Build: `cargo build`
3. Run: `cargo run -- --port /dev/ttyACM0 --program ../test.p [--halfcycles]`

## Expected result

If everything is connected correctly, the program should produce
output like the one shown below. It executes a tiny program that adds two
16-bit numbers together (see the [source](./test.asm)). Each line contains
all the relevant fields, so you can scroll freely without losing the column headers.

```text
Cycle=000009 Half=H Addr=$FFFC Data=$00 RW=R SYNC=0 VP=0 IRQ=1 NMI=1 RES=1
Cycle=000010 Half=H Addr=$FFFD Data=$02 RW=R SYNC=0 VP=0 IRQ=1 NMI=1 RES=1
Cycle=000011 Half=H Addr=$0200 Data=$A9 RW=R SYNC=1 VP=1 IRQ=1 NMI=1 RES=1
Cycle=000012 Half=H Addr=$0201 Data=$03 RW=R SYNC=0 VP=1 IRQ=1 NMI=1 RES=1
Cycle=000013 Half=H Addr=$0202 Data=$8D RW=R SYNC=1 VP=1 IRQ=1 NMI=1 RES=1
Cycle=000014 Half=H Addr=$0203 Data=$FF RW=R SYNC=0 VP=1 IRQ=1 NMI=1 RES=1
Cycle=000015 Half=H Addr=$0204 Data=$FF RW=R SYNC=0 VP=1 IRQ=1 NMI=1 RES=1
Cycle=000016 Half=H Addr=$FFFF Data=$03 RW=W SYNC=0 VP=1 IRQ=1 NMI=1 RES=1
```

The first few logged cycles may differ from run to run - they show the CPU's internal reset
sequence (dummy fetches and three stack accesses) before the reset vector is fetched from
`$FFFC`/`$FFFD`, with the vector pull clearly visible as `VP=0`. From that point on the trace
is fully deterministic.

Running the tools with `--halfcycles` prints both halves of every cycle and appends
`PHI1O`/`PHI2O` at the end of each line for deeper timing inspection.

## How does the example work?

1. The runner loads the binary at $0200, seeds the reset/IRQ/NMI vectors, and asserts reset low for two full cycles.
2. Each half-cycle writes a 40-bit pin snapshot to the bridge, toggling `PHI2` in the payload. The low phase captures
	the address and `RW/` state; the high phase either supplies memory data (reads) or stores results (writes).
3. Every completed bus operation is logged as labeled key/value pairs, so you can follow execution without a logic
	analyzer.

For more details see the development guides for [C++](../docs/development-guide-cpp.md) and
[Rust](../docs/development-guide-rust.md), and the [protocol specification](../docs/protocol.md).
