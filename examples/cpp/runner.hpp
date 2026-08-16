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
    bool step_mode;
    uint64_t steps_remaining;

    void read_serial();
    void write_serial();
    void advance_cycles();
    bool maybe_pause();

public:
    Runner(int device, Memory *mem, bool log_halfcycles, bool step_mode);
    void reset();
    bool step();
    void run();
    void print_state();
};
