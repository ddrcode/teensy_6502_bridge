#pragma once

#include "memory.hpp"
#include "pins.hpp"

#define BUFFSIZE 5

class Runner
{
private:
    Memory *mem;
    bool phase;
    int device;
    uint64_t cycle;
    W65C02Pins pins;

    void read_serial();
    void write_serial();
    void advance_cycles();

public:
    Runner(int device, Memory *mem);
    void reset();
    bool step();
    void run();
    void print_state();
};
