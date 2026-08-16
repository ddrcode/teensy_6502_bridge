# Bridge message protocol

This document specifies the serial protocol spoken between a host computer and the Teensy bridge.
If you want to implement your own host-side client, this page is all you need; reference
implementations in C++ and Rust live in the [examples folder](../examples/).

## Message envelope

The host and the bridge exchange short, framed messages. Every message has the same envelope:

| Offset | Size | Field        | Description                                                          |
| ------ | ---- | ------------ | -------------------------------------------------------------------- |
| 0      | 1    | Message type | See the table below                                                  |
| 1      | N    | Payload      | Size depends on the message type                                     |
| N+1    | 1    | Checksum     | Sum of all preceding bytes (type + payload), wrapping around 8 bits  |

## Message types

| Type | Name   | Payload size | Total size | Description                                                       |
| ---- | ------ | ------------ | ---------- | ----------------------------------------------------------------- |
| 0    | Error  | 1            | 3          | Error code - sent by the bridge when it rejects a message         |
| 1    | Status | 1            | 3          | Reserved for future use                                           |
| 2    | Pins   | 5            | 7          | State of all 40 CPU pins - the only type the host may send        |

In normal operation the communication consists of pins messages (type 2) flowing in both directions:
the host sends the desired state of the pins it controls, the bridge applies it, lets the CPU react,
samples all 40 pins and responds with a single pins message containing the result.

## Validation and errors

The bridge validates every incoming message. When the message type is not a pins message, or the
checksum doesn't match, the bridge leaves the CPU pins untouched and responds with a 3-byte error
message instead of a pins message. Host implementations should therefore read the type byte of a
response first, and only then the rest of it (both example runners do), and should verify the
checksum of every pins response.

| Error code | Meaning                                                             |
| ---------- | ------------------------------------------------------------------- |
| 1          | Invalid message type - the bridge accepts pins messages (2) only    |
| 2          | Checksum mismatch                                                    |

## Pins payload

The payload of a pins message contains the status of all 40 CPU pins (one per bit).
Imagine the CPU pins as a 40-bit number, with pin 1 representing the least significant bit (bit 0)
and pin 40 the most significant bit. That number is transferred in
[big-endian](https://en.wikipedia.org/wiki/Endianness) format, so payload byte 0 (message byte 1)
contains the status of pins 40 to 33 (reading bits left to right), etc.
The table below illustrates the exact structure of the payload.

| Payload byte | Bit 7            | Bit 6             | Bit 5            | Bit 4            | Bit 3            | Bit 2            | Bit 1           | Bit 0           |
| ------------ | ---------------- | ----------------- | ---------------- | ---------------- | ---------------- | ---------------- | --------------- | --------------- |
| 0            | Pin 40<br>`RES/` | Pin 39<br>`PHI2O` | Pin 38<br>`SO/`  | Pin 37<br>`PHI2` | Pin 36<br>`BE`   | Pin 35<br>`NC`   | Pin 34<br>`RW/` | Pin 33<br>`D0`  |
| 1            | Pin 32<br>`D1`   | Pin 31<br>`D2`    | Pin 30<br>`D3`   | Pin 29<br>`D4`   | Pin 28<br>`D5`   | Pin 27<br>`D6`   | Pin 26<br>`D7`  | Pin 25<br>`A15` |
| 2            | Pin 24<br>`A14`  | Pin 23<br>`A13`   | Pin 22<br>`A12`  | Pin 21<br>`VSS`  | Pin 20<br>`A11`  | Pin 19<br>`A10`  | Pin 18<br>`A9`  | Pin 17<br>`A8`  |
| 3            | Pin 16<br>`A7`   | Pin 15<br>`A6`    | Pin 14<br>`A5`   | Pin 13<br>`A4`   | Pin 12<br>`A3`   | Pin 11<br>`A2`   | Pin 10<br>`A1`  | Pin  9<br>`A0`  |
| 4            | Pin  8<br>`VDD`  | Pin  7<br>`SYNC`  | Pin  6<br>`NMI/` | Pin  5<br>`ML/`  | Pin  4<br>`IRQ/` | Pin 3<br>`PHI1O` | Pin  2<br>`RDY` | Pin  1<br>`VP/` |

## Messaging order

As the Teensy bridge doesn't implement a clock, the communication must start on the host side - the
host is responsible for sending the `PHI2` values (pin 37), and - to make the CPU _tick_ - the value
must be inverted in every pins message sent by the host.

Every write to the bridge must be followed by a read, even if you are not planning to use the data
from the bridge (a typical request-response approach). In other words: every single half-cycle
(clock phase) consists of a write to the serial port followed by a read. A full CPU cycle consists
of write-read-write-read operations, with pin 37 set to 0 the first time and to 1 the second time.

During the first half-cycle the CPU executes internal operations, resulting in setting the address
bus and the `RW/` pin. The second half-cycle is a _memory cycle_, when the CPU reads or writes its
data pins. The algorithm below demonstrates the typical interaction with the CPU, respecting both
clock phases.

1. First half-cycle
   1. Set `PHI2` pin to LOW (0) in the payload
   1. Send a pins message to the serial port
   1. Read the 7-byte response from the serial port
   1. Extract the address and the read/write state (pin 34) from the response payload
1. Second half-cycle
   1. Set `PHI2` pin to HIGH (1) in the payload
   1. In case of a read operation (pin 34 is high) - read the value from the memory and set the data pins
   1. Send a pins message to the serial port
   1. Read the 7-byte response from the serial port
   1. In case of a write operation (pin 34 was low in the first half-cycle) - read the value from the data pins and store it in the memory

Note that the bridge drives the CPU data pins only during the high clock phase of read cycles -
like a real memory's output enable gated with `RW/` and `PHI2`. For the entire low phase (and for
write cycles) the data bus is released, so the bus turnaround gap between transactions is
physically real and observable on the bench.

## Timing

No pacing is required on the host side: the bridge replies only after the half-cycle has been fully
handled, so the response itself is the synchronization point, and the host can send the next message
immediately after reading it. The W65C02 is a fully static design, so arbitrarily long gaps between
messages are fine as well - the CPU simply holds its state. In practice the throughput is limited by
the serial round-trip time (around 100 µs per half-cycle over USB), which caps the effective CPU
clock at roughly 5 kHz.
