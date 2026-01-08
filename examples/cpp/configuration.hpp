#include <cstdint>

const uint16_t PROGRAM_ADDR = 0x0200;
const uint16_t INTERRUPT_ADDR = 0x0300;
const uint32_t CYCLE_DURATION = 1000; // microseconds
const bool EXIT_ON_BRK = true;
const uint64_t MAX_CYCLES = 10000;
