// This code makes Teensy 4.1 a bridge betwen COM port and 65C02 processor.
// Created with Teensyduino

#include <_Teensy.h>
#include <string>
#include "configuration.h"

#define BUFFSIZE 5

//--------------------------------------------------------------------------
// TYPES

// W65C02 Pinout:
//
//            +------------+
//     VP <-- |  1      40 | <-- RES
//    RDY <-> |  2      39 | --> PHI2O
//  PHI1O <-- |  3      38 | <-- SO
//    IRQ --> |  4      37 | <-- PHI2
//     ML <-- |  5     @36 | <-- BE
//    NMI --> |  6      35 | --- NC
//   SYNC <-- |  7     *34 | --> RW
//    VDD --> |  8     *33 | <-> D0
//     A0 <-- |  9*    *32 | <-> D1
//     A1 <-- | 10*    *31 | <-> D2
//     A2 <-- | 11*    *30 | <-> D3
//     A3 <-- | 12*    *29 | <-> D4
//     A4 <-- | 13*    *28 | <-> D5
//     A5 <-- | 14*    *27 | <-> D6
//     A6 <-- | 15*    *26 | <-> D7
//     A7 <-- | 16*    *25 | --> A15
//     A8 <-- | 17*    *24 | --> A14
//     A9 <-- | 18*    *23 | --> A13
//    A10 <-- | 19*    *22 | --> A12
//    A11 <-- | 20*     21 | --> GND
//            +------------+
//
//   * - tri-state, / - active on low, @ - async
//
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

void setup()
{
    Serial.begin(8388608); // value ignored on Teensy for USB connection

    // cpu status
    pinMode(pins.rw, INPUT);
    pinMode(pins.sync, INPUT);
    pinMode(pins.vp, INPUT);
    pinMode(pins.ml, INPUT);

    // cpu control
    pinMode(pins.irq, OUTPUT);
    pinMode(pins.nmi, OUTPUT);
    pinMode(pins.reset, OUTPUT);
    pinMode(pins.ready, OUTPUT);
    pinMode(pins.be, OUTPUT);
    pinMode(pins.so, OUTPUT);

    // clock
    pinMode(pins.phi1o, INPUT);
    pinMode(pins.phi2, OUTPUT);
    pinMode(pins.phi2o, INPUT);

    // data
    set_data_pins(pins.data, INPUT);
    for (int i = 0; i < 16; ++i) {
        pinMode(pins.addr[i], INPUT);
    }

    // intialize output pins
    digitalWriteFast(pins.irq, HIGH);
    digitalWriteFast(pins.nmi, HIGH);
    digitalWriteFast(pins.be, HIGH);
    reset();
    digitalWriteFast(pins.ready, HIGH);
    digitalWriteFast(pins.so, HIGH);
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

    digitalWriteFast( pins.reset, LOW);
    delay(CYCLE_DURATION);
    digitalWriteFast( pins.reset, LOW);
    delay(CYCLE_DURATION);
    digitalWriteFast( pins.reset, HIGH);
    delay(CYCLE_DURATION);
}

#ifdef DEBUG_TEENSY_COM_BRIDGE
void print_pin(std::string label, uint8_t pin)
{
    Serial.print(", ");
    Serial.print(label.c_str());
    Serial.print("=");
    Serial.print(digitalRead(pin) == HIGH ? 1 : 0);
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
    auto rw = digitalReadFast(pins.rw);
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
        addr |= digitalReadFast(addr_pins[i]) == HIGH ? (1 << i) : 0;
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
                buff[BUFFSIZE - i - 1] |= digitalReadFast(id) == HIGH ? 1 << j : 0;
            }
        }
    }
}

void set_pins_state(const uint8_t buff[BUFFSIZE])
{
    write_pin(buff, 3); // irq
    write_pin(buff, 5); // nmi
    write_pin(buff, 35); // be
    write_pin(buff, 36); // phi2
    write_pin(buff, 37); // so
    write_pin(buff, 39); // reset

    if (digitalReadFast(pins.rw) == HIGH) {
        for (int i = 32; i > 24; --i) {
            write_pin(buff, i);
        }
    }
}

void write_pin(const uint8_t buff[BUFFSIZE], uint8_t pin_id)
{
    bool val = buff[(39-pin_id) / 8] & (1 << (pin_id % 8));
    digitalWriteFast(pin_ids[pin_id], val ? HIGH : LOW);
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
