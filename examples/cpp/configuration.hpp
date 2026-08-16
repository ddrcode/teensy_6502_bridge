#include <cstdint>

const uint16_t PROGRAM_ADDR = 0x0200;
const uint16_t INTERRUPT_ADDR = 0x0300;
// Extra delay per half-cycle in microseconds. The bridge replies only after
// the half-cycle is complete, so no delay is needed for correctness - increase
// this value to deliberately slow down execution (i.e. to observe board LEDs).
const uint32_t CYCLE_DURATION = 0;
const bool EXIT_ON_BRK = true;
const uint64_t MAX_CYCLES = 10000;
