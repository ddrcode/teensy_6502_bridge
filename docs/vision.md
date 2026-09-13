# Vision: one board, two modes

This project began as - and its name still says - a *bridge*: a Teensy 4.1 that hands full
control of a physical W65C02 to software running on a host computer. That mode is mature,
tested and in daily use. But the board that grew around the bridge - a 600 MHz
microcontroller, a real CPU, a screen, an Ethernet jack - turned out to be provisioned for
something more. This document names the two ways the board can be used, shows that they are
one architecture rather than two projects, and sketches where that leads.

## Mode 1 - the bridge (what exists)

```
 host machine  ── USB serial ──>  Teensy  ── 40 pins ──>  W65C02
 emulated world                   transparent             real CPU
 + the clock                      servant
```

The host owns everything: it drives the clock one half-cycle per message, emulates the
memory and peripherals, and sees the state of all 40 CPU pins in every reply (see the
[protocol specification](./protocol.md)). The CPU runs at a few kHz - limited by USB
round-trip latency - which is not a flaw but the point: every cycle is observable,
steppable and bit-for-bit reproducible.

This is the project's laboratory identity: cycle-by-cycle debugging, lockstep validation of
cycle-accurate emulators, and silicon-in-the-loop test suites. Everything in this repository
serves Mode 1 today.

## Mode 2 - the machine (what could exist)

```
                Teensy  ── 40 pins ──>  W65C02
                emulated world           real CPU
                + the clock
                + screen, keyboard,
                  storage, network
```

No serial connection and no host: the Teensy *is* the computer. It free-runs the clock,
serves the address space from its own RAM, maps ROM images from flash, and emulates the
peripherals - with the real 6502 as the one component that isn't emulated. The concept is
proven by the [Neo6502](https://www.olimex.com/Products/Retro-Computers/Neo6502/open-source-hardware) -
which the main README has cited as an inspiration since 2023. Mode 2 was always purpose #2
in the README; it just never had firmware.

The board is better prepared for this than its name suggests:

| Resource | Mode 2 role |
| -------- | ----------- |
| 600 MHz Cortex-M7 | ~600 microcontroller cycles per CPU cycle at a 1 MHz bus - room for memory *and* device emulation; 1-3 MHz looks feasible bit-banged (the RP2040-based Neo6502 reaches ~6 MHz with PIO assist) |
| 512 KB tightly-coupled RAM | The entire 64 KB address space with single-cycle access, several times over |
| 8 MB flash | ROM images (a C64 kernal is 8 KB; this fits a thousand of them) |
| PSRAM pads (up to 16 MB) | RAM expansions - an emulated REU, banked memory for the '816 |
| ILI9341 320×240 | The C64's 320×200 display fits **pixel-perfect**, with a 40-pixel status strip to spare |
| Ethernet (PCB provision) | File transfer, remote disks, network-era retro experiments |
| USB host (Teensy pads, unrouted on PCB v1.0) | A real keyboard - the missing piece; a v1.1 header candidate (#15) |

## Not two projects: the probe and the target

The unifying observation: **Mode 1 is the built-in debug probe of Mode 2's computer.** That
is the architecture of every modern dev board - a target plus an on-board debugger - except
here the target is a 1975 CPU and the probe speaks cycle-exact truth.

Concretely, one firmware with two loops over the shared pin layer:

- **Standalone**: boot into machine mode; the world runs at speed.
- **Attach**: a host opens the serial port (DTR) - the machine freezes mid-cycle (the W65C02
  is fully static, so "freeze" is literal and lossless) and the bridge protocol takes over.
  The host now steps *the actual running computer*, with its live memory image, on the
  front panel and in the trace.
- **Detach**: DTR drops; the machine resumes as if nothing happened.

Under this reading the project's name survives: the board bridges a real CPU to an emulated
world - Mode 1 keeps that world on the host, Mode 2 moves it onto the Teensy. Which side of
the USB cable the world lives on was never part of the name.

## What the reframe explains

Several design questions that looked like bugs are really *mode mismatches*:

- **The reset button** (#22): reset is clock-synchronous, so a button is meaningless while
  the clock is host-driven and idle (Mode 1) - but works exactly as intended against a
  free-running clock (Mode 2). The v1.0 DS1818 circuit, including its 150 ms debounce
  stretch, was a perfectly reasonable *Mode 2* reset circuit installed in a Mode 1 project.
- **The NMI button** (#15): same story - natural in Mode 2, where presses occur against a
  ticking application.
- The proposed button circuits (pull-up + button + Teensy open-drain, firmware observing
  the net) are deliberately **mode-agnostic**: in Mode 2 the buttons work electrically the
  classic way *and* the firmware sees them - which matters, because a reset that restarts
  the CPU without resetting the emulated chipset would desynchronize the machine. In Mode 1
  the same wiring lets the firmware escort a button press with clock cycles.

## Direction, not commitment

Mode 1 remains the core of this repository and stays stable - it is the identity the test
suites, the sibling projects and the documentation depend on. Mode 2 is an unexplored
direction with an honest sequencing already visible:

1. **W65C816 support** (#3) - firmware pin modes, the groundwork for bigger machines.
2. **A machine-mode prototype** - free-running clock + RAM/ROM from the Teensy, no devices:
   the memory-on-Teensy experiment, which doubles as the bridge's own speed unlock.
3. **Devices** - the ILI9341 as a memory-mapped display, keyboard via a USB-host header
   (v1.1), storage and network later.
4. **The attach/detach probe** - the moment the two modes become one instrument.

None of this is promised. But a board with a real 6502, a screen and a network jack should
not have to pretend it is only a cable.
