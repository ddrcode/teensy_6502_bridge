/*
   Pins handling API

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
#include <cstdint>

#include "../configuration.h"
#include "pins.hpp"

void set_pin_mode(uint8_t pin_id, int mode)
{
    if (pin_id != 255) {
        pinMode(pin_id, mode);
    }
}

void set_data_pins_mode(uint8_t data[8], int direction)
{
    for (int i = 0; i < 8; ++i) {
        pinMode(data[i], direction);
    }
}

uint16_t get_val_from_pins(uint8_t addr_pins[], int len)
{
    int addr = 0;
    for (int i = 0; i < len; ++i) {
        addr |= read_pin(addr_pins[i]) == HIGH ? (1 << i) : 0;
    }
    return addr;
}

void get_pins_state(uint8_t pin_ids[], uint8_t buff[BUFFSIZE])
{
    for(int i=0; i<BUFFSIZE; ++i) {
        buff[BUFFSIZE - i - 1] = 0;
        for(int j=0; j < 8; ++j) {
            auto id = pin_ids[i*8 + j];
            if (id > 0) {
                buff[BUFFSIZE - i - 1] |= read_pin(id) == HIGH ? 1 << j : 0;
            }
        }
    }
}

void set_pins_state(uint8_t pin_ids[], pins_t& pins, const uint8_t buff[BUFFSIZE])
{
    write_pin(pin_ids, 3,  buff); // irq
    write_pin(pin_ids, 5,  buff); // nmi
    write_pin(pin_ids, 35, buff); // be
    write_pin(pin_ids, 36, buff); // phi2
    write_pin(pin_ids, 37, buff); // so
    write_pin(pin_ids, 39, buff); // reset

    if (read_pin(pins.rw) == HIGH) {
        for (int i = 32; i > 24; --i) {
            write_pin(pin_ids, i, buff);
        }
    }
}

int read_pin(const uint8_t pin_id)
{
    return pin_id == 255 ? LOW : digitalReadFast(pin_id);
}

void write_pin(const uint8_t pin_id, const int val)
{
    if (pin_id != 255) {
        digitalWriteFast(pin_id, val);
    }
}

void write_pin(uint8_t pin_ids[], const uint8_t pin_id, const uint8_t buff[BUFFSIZE])
{
    uint8_t id = pin_ids[pin_id];
    if (id != 255) {
        bool val = buff[(39-pin_id) / 8] & (1 << (pin_id % 8));
        digitalWriteFast(id, val ? HIGH : LOW);
    }
}

void set_pin_ids(uint8_t ids[])
{
    uint8_t x[40] = { PINS_MAP };
    for (int i = 0; i < 40; i += 2) {
        ids[i / 2] = x[i];
        ids[39 - i / 2] = x[i + 1];
    }
}

pins_t setup_pins(uint8_t pin_ids[])
{
    set_pin_ids(pin_ids);
    uint8_t *p = pin_ids;

    pins_t pins = {
        .ready = p[1],
        .ml = p[4],
        .be = p[35],
        .so = p[37],
        .reset = p[39],
        .phi1o = p[2],
        .phi2 = p[36],
        .phi2o = p[38],
        .irq = p[3],
        .nmi = p[5],
        .rw = p[33],
        .addr = { p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[16], p[17], p[18], p[19], p[21], p[22], p[23], p[24] },
        .data = { p[32], p[31], p[30], p[29], p[28], p[27], p[26], p[25] },
        .sync = p[6],
        .vp = p[0]
    };
    return pins;
}

