#include <cassert>
#include "hardware.hpp"

mocked_pin_t mocked_pins[MOCKED_PINS_SIZE];

void pinMode(int pin_id, int dir) {
    assert(pin_id >= 0 && pin_id < MOCKED_PINS_SIZE);
    mocked_pins[pin_id].dir = dir;
}

void digitalWriteFast(int pin_id, int val) {
    assert(pin_id >= 0 && pin_id < MOCKED_PINS_SIZE);
    mocked_pins[pin_id].val = val;
}

int digitalReadFast(int pin_id) {
    assert(pin_id >= 0 && pin_id < MOCKED_PINS_SIZE);
    return mocked_pins[pin_id].val;
}

void delay(int _ms) {
}
