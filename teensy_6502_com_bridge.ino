/*
   This code makes Teensy 4.1 a bridge betwen serial port and 65C02 processor.

   Created with Teensyduino
   Author: ddrcode
   Repository: https://github.com/ddrcode/teensy_6502_bridge/

   Compilation: make build
   Upload to Teensy: make upload

   The MIT License
   Copyright (C) 2023-2024 ddrcode

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the “Software”),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <_Teensy.h>
#include <string>

#include "configuration.h"
#include "src/protocol.hpp"
#include "src/pins.hpp"
#include "src/cpu.hpp"
#include "src/io.hpp"

#ifdef DEBUG_TEENSY_BRIDGE
    #include "src/debug.hpp"
#endif

uint8_t pin_ids[40];
pins_t pins = setup_pins(pin_ids);
uint8_t buff[5];

void setup()
{
    Serial.begin(8388608); // value ignored on Teensy for USB connection

    setup_cpu(pins);
    // intialize output pins
    write_pin(pins.irq, HIGH);
    write_pin(pins.nmi, HIGH);
    write_pin(pins.be, HIGH);
    reset(pins);
    write_pin(pins.ready, HIGH);
    write_pin(pins.so, HIGH);
}

void loop()
{
    // do nothihng until serial connected
    if (!Serial.dtr()) {
        return;
    }

#ifdef DEBUG_TEENSY_BRIDGE
    loop_debug(pins);
#else
    loop_prod();
#endif
}

void loop_prod()
{
    static int idx = 0;
    if (!Serial.available()) return;

    int byte = Serial.read();
    buff[idx++] = byte;

    if (idx < 5) return;
    idx = 0;
    set_pins_state(pin_ids, pins, buff);
    handle_cycle(pins);
    get_pins_state(pin_ids, buff);
    msg_pins_t msg = create_pins_msg(buff);
    Serial.write((uint8_t*)&msg, sizeof(msg));
    // Serial.write(buff, BUFFSIZE);
    Serial.send_now();
}
