#pragma once

#include <cstdint>

class W65C02Pins {
    public:
        bool vector_pull;
        bool ready;
        bool phi1o;
        bool irq;
        bool memory_lock;
        bool nmi;
        bool sync;
        uint16_t addr;
        uint8_t data;
        bool rw;
        bool bus_enable;
        bool phi2;
        bool set_overflow;
        bool phi2o;
        bool reset;

        W65C02Pins();
        W65C02Pins(uint8_t pins[5]);

        bool is_write();
        void set_pins(uint8_t buff[5]);
        void set_buff(uint8_t buff[5]);
};
