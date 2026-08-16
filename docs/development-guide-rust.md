# Rust development guide

This guide covers the Rust host runner in [examples/rust](../examples/rust/) - a workstation-side
program that executes a 6502 binary on the real CPU while emulating 64 kB of RAM on the host.
(The bridge firmware itself is written in C++ - see the [C++ guide](./development-guide-cpp.md);
the Rust and C++ runners are functionally identical, so pick whichever toolchain you prefer.)

For the wire format, see the [protocol specification](./protocol.md).

## Source layout

| File | Responsibility |
| ---- | -------------- |
| `src/main.rs`          | CLI parsing (clap), memory initialization and vector seeding, serial-port opening (DTR/RTS asserted) |
| `src/runner.rs`        | The clock loop: reset sequence, half-cycle stepping, memory reads/writes, logging, BRK detection, error-reply handling |
| `src/pins.rs`          | The `Pins` struct - a decoded view of the 40 CPU pins, convertible to/from the 5-byte payload |
| `src/protocol.rs`      | Generic `Message<SIZE>` framing with checksum computation and validation; `PinsMsg = Message<5>` |
| `src/configuration.rs` | Knobs: per-half-cycle delay, raw-message logging |

## Building and running

```bash
cd examples/rust
cargo build
cargo run -- --port /dev/ttyACM0 --program ../test.p [--halfcycles]

# or from the repository root:
PORT=/dev/ttyACM0 make run-rust
```

On macOS the port is named `/dev/cu.usbmodem*` - see the [macOS guide](./macos.md).

CLI flags:

- `--port` / `-p` - the serial device.
- `--program` / `-f` - path to a plain 6502 binary; it is loaded at `$0200` and the reset, IRQ and
  NMI vectors are seeded automatically.
- `--halfcycles` - log both halves of every clock cycle and include the `PHI1O`/`PHI2O` pins.

## Configuration knobs (`src/configuration.rs`)

- `CYCLE_DURATION` - extra delay per half-cycle. `Duration::ZERO` (the default) runs at full speed;
  the bridge's response is the synchronization point, so no delay is needed for correctness.
  Increase it to watch execution in slow motion (e.g. to observe board LEDs).
- `SHOW_RAW_DATA` - print every raw message (type, payload bytes, checksum) as it is sent and
  received; useful when debugging the protocol itself.

## How the runner works

1. `reset()` holds the CPU's `RES/` pin low for two full cycles (four half-cycle messages), then
   releases it.
2. `step()` executes one half-cycle: it toggles `PHI2` in the payload, sends a pins message and
   reads the response. During the low phase it captures the address and the `RW/` state; during the
   high phase it either supplies data from the emulated memory (reads) or stores the data pins into
   it (writes).
3. The loop stops when the CPU fetches a `BRK` opcode (`SYNC` high with data `$00`).

The runner reads the type byte of every response first: a pins message (type 2) is 7 bytes, while an
error reply from the bridge (type 0) is only 3 bytes and makes the runner panic with the reported
error code. Checksums of all pins responses are validated.

## Tests

```bash
cd examples/rust
cargo test
```

The tests cover the payload/`Pins` conversions and the message framing - pure logic, no hardware or
serial port required.

## Writing test programs

Any 6502 assembler producing a plain binary works. The examples use [ACME](https://github.com/meonwax/acme)
(available in the project's Nix shell):

```bash
acme -f plain -o program.p --cpu w65c02 program.asm
```

End the program with `BRK` so the runner stops cleanly - see
[examples/test.asm](../examples/test.asm) for a complete, commented example.

## Writing your own client

`runner.rs` is intentionally small - if you want to build something else on top of the bridge
(a debugger UI, an emulator comparator, a whole emulated machine), the pieces to reuse are
`protocol.rs` and `pins.rs`, plus the half-cycle algorithm from the
[protocol specification](./protocol.md). Everything the bridge can do is reachable through that
one message type.
