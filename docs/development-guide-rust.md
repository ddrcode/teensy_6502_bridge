# Rust development guide

This guide covers the two Rust parts of the project: the bare-metal bridge firmware in
[firmware-rust](../firmware-rust/) and the host runner in [examples/rust](../examples/rust/).
For the wire format they share, see the [protocol specification](./protocol.md).

## Firmware (firmware-rust)

An alternative to the C++ firmware in the repository root, functionally equivalent on the wire:
the same message framing, checksum validation, error replies and data-bus gating (the bus is
driven only during the high clock phase of read cycles). The host runners and test programs work
with either firmware unchanged. Built on [teensy4-rs](https://github.com/mciantyre/teensy4-rs)
(`teensy4-bsp` + `imxrt-usbd` + `usbd-serial`), plain stable Rust - no Arduino toolchain involved.

The Rust firmware currently has no debug/diagnostics modes and no ILI9341 display support - when
you need those, flash the C++ firmware. To tell them apart, the Rust firmware enumerates as
`W65C02 Bridge (Rust)`.

### Source layout

| File | Responsibility |
| ---- | -------------- |
| `src/main.rs`        | Board and GPIO setup, USB CDC plumbing, and the request-response loop |
| `src/protocol.rs`    | Message framing: sizes, checksums, error replies, the byte-stream accumulator (host-tested) |
| `src/payload.rs`     | The 40-bit pin payload bit layout (host-tested, incl. a real hardware-captured vector) |
| `src/lib.rs`         | The hardware-independent logic: `no_std` on the target, `std` for host tests |
| `Makefile`           | `build` / `hex` / `flash` / `test` targets |
| `.cargo/config.toml` | Default target (`thumbv7em-none-eabihf`) and the linker script |

### Building, flashing, testing

```bash
cd firmware-rust
make build   # cargo build --release (rustup installs the target on first run)
make hex     # ELF -> teensy-6502-bridge.hex via the toolchain's llvm-objcopy
make flash   # flash with teensy-loader-cli
make test    # host-side unit tests of the protocol/payload logic
```

`make flash` expects a native `teensy-loader-cli` on the PATH (on macOS build it from source -
see the [macOS guide](./macos.md); the nixpkgs build does not work there).

Reflashing needs no button press: the running Rust firmware reboots into the HalfKay bootloader
when the serial port is opened at **134 baud** - the same convention Teensyduino uses:

```bash
/bin/stty -f /dev/cu.usbmodemXXXX 134   # macOS (full path in case GNU coreutils shadows stty)
stty -F /dev/ttyACM0 134                # Linux
```

### Design notes

- **Bare metal with a polling loop, not async.** The protocol is a latency-bound
  request-response exchange over a single peripheral - exactly the shape where a poll loop is
  the right architecture, not a compromise. (`embassy-imxrt` targets the i.MX RT600 family, not
  the Teensy's RT1062; RTIC would be the natural framework if interrupt-driven structure is ever
  needed.)
- **Type-checked pin mapping.** Every GPIO is created through the HAL's typed pad API
  (`teensy4-pins`), so a wrong pin/port pairing fails at build or boot time instead of silently
  writing to the wrong register - the failure mode that killed the first attempt at this
  firmware.
- **SION on all output pads.** The Software Input On bit keeps a driven pad's input path alive,
  so output levels can be read back through PSR. `sample()` and the `RW && PHI2` data-bus gate
  depend on it - without SION, PHI2 always reads low, the data bus is never driven and the CPU
  executes garbage from a floating bus.
- **Data-bus direction flips** use the HAL's documented-reentrant `Output::without_pin` /
  `Input::without_pin` constructors - a single GDIR register write per line.

The [firmware README](../firmware-rust/README.md) repeats the practical parts of this section
next to the code.

## Host runner (examples/rust)

A workstation-side program that executes a 6502 binary on the real CPU while emulating 64 kB of
RAM on the host. The Rust and C++ runners are functionally identical, so pick whichever
toolchain you prefer.

### Source layout

| File | Responsibility |
| ---- | -------------- |
| `src/main.rs`          | CLI parsing (clap), memory initialization and vector seeding, serial-port opening (DTR/RTS asserted) |
| `src/runner.rs`        | The clock loop: reset sequence, half-cycle stepping, memory reads/writes, logging, BRK detection, error-reply handling |
| `src/pins.rs`          | The `Pins` struct - a decoded view of the 40 CPU pins, convertible to/from the 5-byte payload |
| `src/protocol.rs`      | Generic `Message<SIZE>` framing with checksum computation and validation; `PinsMsg = Message<5>` |
| `src/configuration.rs` | Knobs: per-half-cycle delay, raw-message logging |

### Building and running

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
- `--step` - step mode: pause after every logged cycle (or half-cycle, with `--halfcycles`) with the
  CPU frozen, and wait for a command on stdin: `Enter` advances one step, a number runs that many
  steps, `c` switches back to free-running, `q` quits. As the W65C02 is fully static, the CPU simply
  holds its state while paused - handy for inspecting board LEDs or probing signals.

### Configuration knobs (`src/configuration.rs`)

- `CYCLE_DURATION` - extra delay per half-cycle. `Duration::ZERO` (the default) runs at full speed;
  the bridge's response is the synchronization point, so no delay is needed for correctness.
  Increase it to watch execution in slow motion (e.g. to observe board LEDs).
- `SHOW_RAW_DATA` - print every raw message (type, payload bytes, checksum) as it is sent and
  received; useful when debugging the protocol itself.

### How the runner works

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

### Tests

```bash
cd examples/rust
cargo test
```

The tests cover the payload/`Pins` conversions and the message framing - pure logic, no hardware or
serial port required.

### Writing test programs

Any 6502 assembler producing a plain binary works. The examples use [ACME](https://github.com/meonwax/acme)
(available in the project's Nix shell):

```bash
acme -f plain -o program.p --cpu w65c02 program.asm
```

End the program with `BRK` so the runner stops cleanly - see
[examples/test.asm](../examples/test.asm) for a complete, commented example.

### Writing your own client

`runner.rs` is intentionally small - if you want to build something else on top of the bridge
(a debugger UI, an emulator comparator, a whole emulated machine), the pieces to reuse are
`protocol.rs` and `pins.rs`, plus the half-cycle algorithm from the
[protocol specification](./protocol.md). Everything the bridge can do is reachable through that
one message type.
