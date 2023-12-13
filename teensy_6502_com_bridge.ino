// This code makes Teensy 4.1 a bridge betwen COM port and 65C02 processor.
// Created with Teensyduino

#include <_Teensy.h>
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
    int ready;
    int ml;  // memory lock
    int be;  // bus enable
    int so;  // set overflow
    int reset;

    // clock
    int phi1o;
    int phi2;
    int phi2o;

    // interrupts
    int irq;
    int nmi;

    // data
    int rw;
    int addr[16];
    int data[8];

    // monitoring
    int sync;
    int vp;  // vector pull
} pins_t;

pins_t setup_pins(int pins[]);

//--------------------------------------------------------------------------
// GLOBAL DATA

int pin_ids[40];
auto pins = setup_pins(pin_ids);
uint8_t buff[BUFFSIZE];

//--------------------------------------------------------------------------
// PUBLIC API

void setup()
{
    Serial.begin(9600); // value ignored on Teensy for USB connection

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
    digitalWrite(pins.irq, HIGH);
    digitalWrite(pins.nmi, HIGH);
    // digitalWrite(pins.ready, HIGH);
    digitalWrite(pins.be, HIGH);
    // digitalWrite(pins.so, HIGH);
    reset();
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

    digitalWrite(pins.irq, HIGH);
    digitalWrite(pins.nmi, HIGH);
    digitalWrite(pins.ready, HIGH);
    digitalWrite(pins.be, HIGH);
    digitalWrite(pins.so, HIGH);

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

    digitalWrite( pins.reset, LOW);
    delay(CYCLE_DURATION);
    digitalWrite( pins.reset, LOW);
    delay(CYCLE_DURATION);
    digitalWrite( pins.reset, HIGH);
    delay(CYCLE_DURATION);
}

#ifdef DEBUG_TEENSY_COM_BRIDGE
void print_status(pins_t &pins)
{
    Serial.print("RW=");
    Serial.print(digitalRead(pins.rw));
    Serial.print(", Addr=");
    Serial.print(get_val_from_pins(pins.addr, 16), HEX);
    Serial.print(", Data=");
    Serial.print(get_val_from_pins(pins.data, 8), BIN);
    Serial.print(", Pin35=");
    Serial.print(digitalRead(35), BIN);
    Serial.print(", Pin36=");
    Serial.print(digitalRead(36), BIN);
    Serial.println();
}
#endif

//--------------------------------------------------------------------------
// LOCAL FUNCTIONS

void handle_cycle(pins_t &pins)
{
    auto rw = digitalRead(pins.rw);
    set_data_pins(pins.data, rw == HIGH ? OUTPUT : INPUT);
}

void set_data_pins(int data[8], int direction)
{
    for (int i = 0; i < 8; ++i) {
        pinMode(data[i], direction);
    }
}

uint16_t get_val_from_pins(int addr_pins[], int len)
{
    int addr = 0;
    for (int i = 0; i < len; ++i) {
        addr |= digitalRead(addr_pins[i]) == HIGH ? (1 << i) : 0;
    }
    return addr;
}

void get_pins_state(uint8_t buff[BUFFSIZE])
{
    for(int i=0; i<BUFFSIZE; ++i) {
        buff[i] = 0;
        for(int j=0; j < 8; ++j) {
            auto id = pin_ids[i*8 + j];
            if (id > 0) {
                buff[i] |= digitalRead(id) == HIGH ? 1 << j : 0;
            }
        }
    }
}

pins_t setup_pins(int pin_ids[])
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

void set_pin_ids(int ids[40])
{
    int x[40] = { PINS_MAP };
    for (int i = 0; i < 40; i += 2) {
        ids[i / 2] = x[i];
        ids[39 - i / 2] = x[i + 1];
    }
}
