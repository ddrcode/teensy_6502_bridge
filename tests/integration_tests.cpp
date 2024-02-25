#include "tests.hpp"
#include "../src/pins.hpp"
#include "../src/cpu.hpp"
#include "mocks/hardware.hpp"

uint8_t pin_ids[40];
pins_t pins;

void _setup() {
    pins = setup_pins(pin_ids);
    setup_cpu(pins);
    reset(pins);
}

void test_pins_to_msg() {
    _setup();
    assert(read_pin(pins.irq) == HIGH);
    assert(read_pin(pins.reset) == HIGH);

    uint8_t buff[5];
    get_pins_state(pin_ids, buff);

    assert(buff[0] & 128); // reset
    assert(buff[4] & 8); // IRQ
}

void integration_tests_all() {
    test_pins_to_msg();
}
