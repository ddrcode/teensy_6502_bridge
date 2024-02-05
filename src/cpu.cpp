/*
   Operations on 6502/65816 CPU

   Created with Teensyduino
   Author: ddrcode
   Repository: https://github.com/ddrcode/teensy_6502_bridge/

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

#include <Arduino.h>
#include <_Teensy.h>

#include "cpu.hpp"
#include "pins.hpp"
#include "../configuration.h"

//--------------------------------------------------------------------------
// GLOBAL DATA

void setup_cpu(pins_t& pins)
{
    // cpu status
    set_pin_mode(pins.rw, INPUT);
    set_pin_mode(pins.sync, INPUT);
    set_pin_mode(pins.vp, INPUT);
    set_pin_mode(pins.ml, INPUT);

    // cpu control
    set_pin_mode(pins.irq, OUTPUT);
    set_pin_mode(pins.nmi, OUTPUT);
    set_pin_mode(pins.reset, OUTPUT);
    set_pin_mode(pins.ready, OUTPUT);
    set_pin_mode(pins.be, OUTPUT);
    set_pin_mode(pins.so, OUTPUT);

    // clock
    set_pin_mode(pins.phi1o, INPUT);
    set_pin_mode(pins.phi2, OUTPUT);
    set_pin_mode(pins.phi2o, INPUT);

    // data
    set_data_pins_mode(pins.data, INPUT);
    for (int i = 0; i < 16; ++i) {
        set_pin_mode(pins.addr[i], INPUT);
    }
}

void reset(pins_t &pins)
{
    write_pin( pins.reset, LOW);
    delay(CYCLE_DURATION);
    write_pin( pins.reset, LOW);
    delay(CYCLE_DURATION);
    write_pin( pins.reset, HIGH);
    delay(CYCLE_DURATION);
}

void handle_cycle(pins_t &pins)
{
    auto rw = read_pin(pins.rw);
    set_data_pins_mode(pins.data, rw == HIGH ? OUTPUT : INPUT);
}
