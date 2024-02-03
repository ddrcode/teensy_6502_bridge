#pragma once

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

    // interrupts
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

void set_pin_mode(uint8_t pin_id, int mode);
void set_data_pins_mode(uint8_t data[8], int direction);
void get_pins_state(uint8_t buff[BUFFSIZE]);
void set_pins_state(const uint8_t buff[BUFFSIZE]);
inline int read_pin(const uint8_t pin_id);
inline void write_pin(const uint8_t pin_id, const int val);
pins_t setup_pins(uint8_t pin_ids[]);

