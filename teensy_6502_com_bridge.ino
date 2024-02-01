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

//--------------------------------------------------------------------------
// GLOBAL DATA

constexpr int BUFFSIZE = 5;

typedef struct t_w64c02_pins
{
    // cpu control
    uint8_t ready;
    uint8_t ml;  // memory lock
    uint8_t be;  // bus enable
    uint8_t so;  // set overflow
    uint8_t reset;

    // clock
    uint8_t phi1o;
    uint8_t phi2;
    uint8_t phi2o;

    // uint8_terrupts
    uint8_t irq;
    uint8_t nmi;

    // data
    uint8_t rw;
    uint8_t addr[16];
    uint8_t data[8];

    // monitoring
    uint8_t sync;
    uint8_t vp;  // vector pull
} pins_t;

pins_t setup_pins(uint8_t pins[]);

//--------------------------------------------------------------------------
// GLOBAL DATA

uint8_t pin_ids[40];
pins_t pins = setup_pins(pin_ids);
uint8_t buff[BUFFSIZE];

//--------------------------------------------------------------------------
// PUBLIC API

void set_pin_mode(uint8_t pin_id, int mode)
{
    if (pin_id != 255) {
        pinMode(pin_id, mode);
    }
}

void setup()
{
    Serial.begin(8388608); // value ignored on Teensy for USB connection

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
    set_data_pins(pins.data, INPUT);
    for (int i = 0; i < 16; ++i) {
        set_pin_mode(pins.addr[i], INPUT);
    }

    // intialize output pins
    write_pin(pins.irq, HIGH);
    write_pin(pins.nmi, HIGH);
    write_pin(pins.be, HIGH);
    reset();
    write_pin(pins.ready, HIGH);
    write_pin(pins.so, HIGH);
}

void loop()
{
    // do nothihng until serial connected
    if (!Serial.dtr()) {
        return;
    }

#ifdef DEBUG_TEENSY_COM_BRIDGE
    loop_debug();
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
    set_pins_state(buff);
    handle_cycle(pins);
    get_pins_state(buff);
    Serial.write(buff, BUFFSIZE);
    Serial.send_now();
}

#ifdef DEBUG_TEENSY_COM_BRIDGE
void loop_debug()
{
    static int cycle = 0;
    static bool phase = true;

    digitalToggle(pins.phi2);
    delay(CYCLE_DURATION);
    handle_cycle(pins);
    if (phase) print_status(pins);
    ++cycle;
    phase = !phase;
}
#endif

void reset()
{
#ifndef DEBUG_TEENSY_COM_BRIDGE
    static const int CYCLE_DURATION = 50;
#endif

    write_pin( pins.reset, LOW);
    delay(CYCLE_DURATION);
    write_pin( pins.reset, LOW);
    delay(CYCLE_DURATION);
    write_pin( pins.reset, HIGH);
    delay(CYCLE_DURATION);
}

#ifdef DEBUG_TEENSY_COM_BRIDGE
void print_pin(std::string label, uint8_t pin)
{
    Serial.print(", ");
    Serial.print(label.c_str());
    Serial.print("=");
    Serial.print(read_pin(pin) == HIGH ? 1 : 0);
}

void print_status(pins_t &pins)
{
    Serial.print("Addr=");
    Serial.print(get_val_from_pins(pins.addr, 16), HEX);
    Serial.print(", Data=");
    Serial.print(get_val_from_pins(pins.data, 8), BIN);
    print_pin("RW", pins.rw);
    print_pin("SYNC", pins.sync);
    print_pin("VP", pins.vp);
    print_pin("ML", pins.ml);
    Serial.println();
}
#endif

//--------------------------------------------------------------------------
// LOCAL FUNCTIONS

void handle_cycle(pins_t &pins)
{
    auto rw = read_pin(pins.rw);
    set_data_pins(pins.data, rw == HIGH ? OUTPUT : INPUT);
}

void set_data_pins(uint8_t data[8], int direction)
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

void get_pins_state(uint8_t buff[BUFFSIZE])
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

void set_pins_state(const uint8_t buff[BUFFSIZE])
{
    write_pin(3,  buff); // irq
    write_pin(5,  buff); // nmi
    write_pin(35, buff); // be
    write_pin(36, buff); // phi2
    write_pin(37, buff); // so
    write_pin(39, buff); // reset

    if (read_pin(pins.rw) == HIGH) {
        for (int i = 32; i > 24; --i) {
            write_pin(i, buff);
        }
    }
}

inline int read_pin(const uint8_t pin_id)
{
    return pin_id == 255 ? LOW : digitalReadFast(pin_id);
}

inline void write_pin(const uint8_t pin_id, const int val)
{
    if (pin_id != 255) {
        digitalWriteFast(pin_id, val);
    }
}

void write_pin(const uint8_t pin_id, const uint8_t buff[BUFFSIZE])
{
    auto id = pin_ids[pin_id];
    if (id != 255) {
        bool val = buff[(39-pin_id) / 8] & (1 << (pin_id % 8));
        digitalWriteFast(id, val ? HIGH : LOW);
    }
}

pins_t setup_pins(uint8_t pin_ids[])
{
    set_pin_ids(pin_ids);
    auto p = pin_ids;

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

void set_pin_ids(uint8_t ids[40])
{
    uint8_t x[40] = { PINS_MAP };
    for (int i = 0; i < 40; i += 2) {
        ids[i / 2] = x[i];
        ids[39 - i / 2] = x[i + 1];
    }
}
