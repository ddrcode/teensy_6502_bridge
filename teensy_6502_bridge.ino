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

#include "src/hardware.hpp"
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
uint8_t buff[7];

void setup()
{
    Serial.begin(8388608); // value ignored on Teensy for USB connection
    setup_cpu(pins);
    reset(pins);
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
    // static int idx = 0;
    // static message_t msg = {
    //     .size = 5
    // };

    if (!Serial.available()) return;

    message_t msg = read_msg();

    // int byte = Serial.read();
    // buff[idx++] = byte;
    //
    // if (idx < 7) return;
    // idx = 0;
    //
    // msg.type = buff[0];
    // for(int i=0; i<5; ++i) {
    //     msg.data[i] = buff[i+1];
    // }
    // msg.checksum = buff[6];

    set_pins_state(pin_ids, pins, msg.data);
    handle_cycle(pins);
    get_pins_state(pin_ids, msg.data);
    // write_msg(&msg);
    msg.checksum = compute_checksum(&msg);
    msg_to_buff(&msg, buff);
    Serial.write(buff, 7);
    Serial.send_now();

    // set_pins_state(pin_ids, pins, buff+1);
    // handle_cycle(pins);
    // get_pins_state(pin_ids, buff+1);
    // msg_pins_t msg = create_pins_msg(buff+1);
    // Serial.write((uint8_t*)&msg, sizeof(msg));
    // // Serial.write(buff, 5);
    // Serial.send_now();
}
