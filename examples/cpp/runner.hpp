#pragma once

#include "memory.hpp"
#include "pins.hpp"

class Runner
{
private:
    Memory *mem;
    bool phase;
    int device;
    uint64_t cycle;
    W65C02Pins pins;
    bool log_halfcycles;

    void read_serial();
    void write_serial();
    void advance_cycles();

public:
    Runner(int device, Memory *mem, bool log_halfcycles);
    void reset();
    bool step();
    void run();
    void print_state();
};
