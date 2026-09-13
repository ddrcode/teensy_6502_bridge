# W65C02-Teensy Bridge - Rust firmware

A bare-metal Rust implementation of the bridge firmware, functionally equivalent
to the C++ firmware in the repository root: the same
[serial protocol](../docs/protocol.md) (framed messages, checksum validation,
error replies), the same data-bus gating (driven only during the high clock
phase of read cycles), and the same high-speed USB. The host runners and test
programs work with either firmware unchanged.

Built on [teensy4-rs](https://github.com/mciantyre/teensy4-rs) (`teensy4-bsp`),
with USB CDC via `imxrt-usbd` + `usbd-serial`. No Arduino, no arduino-cli.

## Extras over the C++ firmware

- **Self-rebooting flash**: opening the serial port at **134 baud** reboots the
  board into the HalfKay bootloader (the same convention Teensyduino uses), so
  reflashing needs no button press:

  ```bash
  stty -f /dev/cu.usbmodemXXXX 134    # macOS (use /bin/stty if GNU coreutils shadows it)
  stty -F /dev/ttyACM0 134            # Linux
  ```

- The device enumerates as `W65C02 Bridge (Rust)` (serial `6502-RS-2`), so it is
  easy to tell which firmware is flashed.

## Building and flashing

```bash
cd firmware-rust
make build    # cargo build --release (rustup installs the target automatically)
make hex      # ELF -> teensy-6502-bridge.hex (llvm-objcopy from the toolchain)
make flash    # flash with teensy-loader-cli (see below)
make test     # host-side unit tests of the protocol/payload logic
```

`make flash` expects a native `teensy-loader-cli` on the PATH (on macOS build it
from source - see [docs/macos.md](../docs/macos.md); the nixpkgs build does not
work there). If the Rust firmware is already running, trigger the 134-baud
reboot first (or let `teensy-loader-cli -w` wait and press the button).

## Source layout

| File | Responsibility |
| ---- | -------------- |
| `src/main.rs`     | Board/GPIO/USB setup and the request-response loop |
| `src/protocol.rs` | Message framing: sizes, checksums, error replies, byte-stream accumulator (host-tested) |
| `src/payload.rs`  | The 40-bit pin payload bit layout (host-tested, incl. a real hardware-captured vector) |
| `src/lib.rs`      | `no_std` on target, `std` for host tests |

## Hardware notes (learned the hard way)

- Pin mapping comes from the `teensy4-pins` pad table - every pin is created
  through the HAL's type-checked API, so a wrong pin/port pairing fails at
  build/boot rather than silently writing to the wrong register.
- All output pads are configured with **SION** (Software Input On), so the
  driven levels can be read back through PSR. Without it, `handle_cycle`'s
  `RW && PHI2` bus gate reads PHI2 as always-low and the data bus is never
  driven - the CPU then executes garbage from a floating bus.
- Data-bus direction is flipped by re-running the HAL's documented-reentrant
  `Output::without_pin`/`Input::without_pin` constructors (a single GDIR
  register write per line).

## Limitations vs the C++ firmware

- No debug/diagnostics modes and no ILI9341 display support (the display could
  be added later with `embedded-graphics` + an `ili9341` driver crate).
- Async (embassy) is not used: `embassy-imxrt` targets the i.MX RT600 family,
  not the Teensy's RT1062. A polling loop is the right shape for this
  latency-bound protocol anyway; RTIC would be the natural framework if
  interrupt-driven structure is ever needed.

## Verification

The firmware passes the same hardware test battery as the C++ one: protocol
probe (checksum/type error replies, valid pins exchange), deterministic
cycle-exact soak runs, and the `examples/test.p` correctness check - at the
same throughput (~5,000 CPU cycles/s over high-speed USB).
