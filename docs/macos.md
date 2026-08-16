# Running the bridge on macOS

The project was developed on Linux, but everything works on macOS too — the serial
ports just have different names, and uploading the firmware needs a different tool.
This page collects everything Darwin-specific.

## Finding the serial port

When the Teensy runs the bridge firmware, it appears as `/dev/cu.usbmodemXXXXXXXXX`
(instead of Linux's `/dev/ttyACM0`):

```bash
ls /dev/cu.usbmodem*
# or
make detect-port          # wraps `arduino-cli board list`
```

Two things to know:

- Use the `/dev/cu.*` device, not its `/dev/tty.*` twin. On macOS the `tty.*`
  variant blocks on open until a carrier signal appears; `cu.*` (the "call-out"
  device) opens immediately, which is what the host runners expect.
- The number in the device name identifies the physical USB port, so it changes
  when you plug the board into a different socket.

Once you know the port:

```bash
PORT=/dev/cu.usbmodemXXXXXXXXX make run-rust    # or run-cpp
```

## Cables and hubs

Prefer connecting the Teensy directly to the computer. Some USB hubs (and all
charge-only cables) prevent the board from enumerating at all — if the board is
invisible in `ioreg -p IOUSB` even in bootloader mode (button pressed), suspect
the path to the machine before suspecting the firmware.

## Uploading the firmware

### Why `make upload` may fail

`make upload` calls `teensy-loader-cli`. If yours comes from nixpkgs (as with this
project's `shell.nix`) or is any other build compiled against libusb, it fails on
macOS like this:

```text
Waiting for Teensy device...
Device is in use by "dummy" driver
Unable to claim interface, check USB permissions
```

Despite the hint, this is not a permission problem (macOS has no udev equivalent,
and doesn't need one). The Teensy bootloader is a USB HID device, which macOS's
own HID driver claims automatically. The libusb code path then tries the Linux
trick of detaching the kernel driver — an operation that doesn't exist on macOS —
and gives up. (The name "dummy" is a placeholder returned by libusb-compat, which
cannot even query the real driver's name on Darwin.) Loaders built with Apple's
native IOKit HID API coexist with the Apple driver and work without any tricks;
that's what both options below use.

### Option 1 — PJRC tools from the Teensy Arduino core

If you installed the `teensy:avr` core for `arduino-cli` (see the main README),
the native tools are already on your disk:

```bash
TOOLS=$(ls -d ~/Library/Arduino15/packages/teensy/tools/teensy-tools/* | sort -V | tail -1)

make build
open -g "$TOOLS/teensy.app"     # start Teensy Loader in the background
"$TOOLS/teensy_post_compile" -file=teensy_6502_bridge.ino -path="$PWD/target" \
    -tools="$TOOLS" -board=TEENSY41 -reboot
```

Press the button on the board if the loader waits for it. On Apple Silicon these
tools are x86_64 binaries, so Rosetta 2 must be installed
(`softwareupdate --install-rosetta`).

### Option 2 — build `teensy_loader_cli` natively

The upstream `teensy_loader_cli` works fine on macOS when compiled with its
IOKit backend (a 30-second build):

```bash
git clone https://github.com/PaulStoffregen/teensy_loader_cli
cd teensy_loader_cli
make OS=MACOSX      # inside this project's nix-shell add: CC=/usr/bin/clang
```

Then, from the project directory:

```bash
make build
path/to/teensy_loader_cli --mcu=TEENSY41 -w -v target/teensy_6502_bridge.ino.hex
```

The GUI [Teensy Loader](https://www.pjrc.com/teensy/loader_mac.html) from PJRC is
a third alternative if you prefer clicking.

## Recognizing the bootloader

Pressing the button puts the board into the HalfKay bootloader. Two things
surprise people:

- The bootloader is **not** a serial port — `/dev/cu.usbmodem*` disappears while
  the board sits in bootloader mode. That's normal; the port comes back once new
  firmware is flashed and the board reboots.
- The bootloader publishes no product-name string, so it's easy to miss in USB
  listings. Look for the PJRC vendor/product IDs instead:

```bash
ioreg -p IOUSB -w0 -l | grep -E 'idVendor" = 5824|idProduct" = 1144'
# 5824 = 0x16C0 (PJRC), 1144 = 0x0478 (HalfKay bootloader)
```

If those IDs show up, the board and the cable are fine — everything is ready for
flashing.
