# Example - program runner

The program in this folder executes 6502 binary, using the
Teensy bridge. As 6502 needs RAM for functioning, the memory
is emulated with a simple byte array in the program (on the host machine).

This folder provides two implementations of the example (doing exactly the same):
one in C++ and one in Rust.

## Compilation and execution

For both, C++ and Rust program, make sure you connect your Teensy with running Bridge
app from this repo first. The microcontroller must be connected in order to make the
host computer recognise the serial port.

It's sufficient to have the CPU connected only with address, data,
Phi2 and Read/Write pins.

### C++ version

1. Check Teensy's port name (`arduino-cli board list`)
   and update [`configuration.hpp`](cpp/configuration.hpp) file, if needed.
1. Compile the example: `g++ -o example.out *.cpp`
1. Execute the example app: `./example.out`

### Rust version

1. Check Teensy's port name (`arduino-cli board list`)
   and update [`configuration.rs`](rust/src/configuration.rs) file, if needed.
1. Compile the example: `cargo build`
1. Execute the example app: `cargo run`

## Expected result

If everything is connected correctly, the program should produce
an output as showed below. It executes a tiny program, that adds two
16-bit numbers together (see the [source](./test.asm)).

```text
[1] Addr: 0000, Data: 00, Flags (VP, SYNC, RW): 000, Inputs (PHI, IRQ, NMI, RST): 0000
[2] Addr: 022b, Data: ea, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[3] Addr: 022b, Data: ea, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[4] Addr: ffff, Data: 00, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[5] Addr: 022c, Data: 4c, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[6] Addr: 01e4, Data: 00, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[7] Addr: 01e3, Data: 00, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[8] Addr: 01e2, Data: 00, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[9] Addr: fffc, Data: 00, Flags (VP, SYNC, RW): 001, Inputs (PHI, IRQ, NMI, RST): 1111
[10] Addr: fffd, Data: 02, Flags (VP, SYNC, RW): 001, Inputs (PHI, IRQ, NMI, RST): 1111
[11] Addr: 0200, Data: a0, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[12] Addr: 0201, Data: 01, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[13] Addr: 0202, Data: a9, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[14] Addr: 0203, Data: 05, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[15] Addr: 0204, Data: 85, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[16] Addr: 0205, Data: 10, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[17] Addr: 0010, Data: 05, Flags (VP, SYNC, RW): 100, Inputs (PHI, IRQ, NMI, RST): 1111
[18] Addr: 0206, Data: a9, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[19] Addr: 0207, Data: ff, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[20] Addr: 0208, Data: 85, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[21] Addr: 0209, Data: 11, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[22] Addr: 0011, Data: ff, Flags (VP, SYNC, RW): 100, Inputs (PHI, IRQ, NMI, RST): 1111
[23] Addr: 020a, Data: a9, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[24] Addr: 020b, Data: da, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[25] Addr: 020c, Data: 85, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[26] Addr: 020d, Data: 12, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[27] Addr: 0012, Data: da, Flags (VP, SYNC, RW): 100, Inputs (PHI, IRQ, NMI, RST): 1111
[28] Addr: 020e, Data: a9, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[29] Addr: 020f, Data: 21, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[30] Addr: 0210, Data: 85, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[31] Addr: 0211, Data: 13, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[32] Addr: 0013, Data: 21, Flags (VP, SYNC, RW): 100, Inputs (PHI, IRQ, NMI, RST): 1111
[33] Addr: 0212, Data: 18, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[34] Addr: 0213, Data: a5, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[35] Addr: 0213, Data: a5, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[36] Addr: 0214, Data: 11, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[37] Addr: 0011, Data: ff, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[38] Addr: 0215, Data: 65, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[39] Addr: 0216, Data: 13, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[40] Addr: 0013, Data: 21, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[41] Addr: 0217, Data: 85, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[42] Addr: 0218, Data: 15, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[43] Addr: 0015, Data: 20, Flags (VP, SYNC, RW): 100, Inputs (PHI, IRQ, NMI, RST): 1111
[44] Addr: 0219, Data: aa, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[45] Addr: 021a, Data: a5, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[46] Addr: 021a, Data: a5, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[47] Addr: 021b, Data: 10, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[48] Addr: 0010, Data: 05, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[49] Addr: 021c, Data: 65, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[50] Addr: 021d, Data: 12, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[51] Addr: 0012, Data: da, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[52] Addr: 021e, Data: 85, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[53] Addr: 021f, Data: 14, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[54] Addr: 0014, Data: e0, Flags (VP, SYNC, RW): 100, Inputs (PHI, IRQ, NMI, RST): 1111
[55] Addr: 0220, Data: c9, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[56] Addr: 0221, Data: e0, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[57] Addr: 0222, Data: d0, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[58] Addr: 0223, Data: 07, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[59] Addr: 0224, Data: e0, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[60] Addr: 0225, Data: 20, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[61] Addr: 0226, Data: d0, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[62] Addr: 0227, Data: 03, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[63] Addr: 0228, Data: a0, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
[64] Addr: 0229, Data: 00, Flags (VP, SYNC, RW): 101, Inputs (PHI, IRQ, NMI, RST): 1111
[65] Addr: 022a, Data: 00, Flags (VP, SYNC, RW): 111, Inputs (PHI, IRQ, NMI, RST): 1111
```

## How does the example work?
TBD
