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

void _teardown() {
    _reset_hardware_mocks();
    Serial._reset();
}

void _run_test(void (* const test_fn)()) {
    _setup();
    test_fn();
    _teardown();
}

void test_reset() {
    reset(pins);
    assert(read_pin(pins.reset) == HIGH);
    assert(read_pin(pins.nmi) == HIGH);
    assert(read_pin(pins.irq) == HIGH);
    assert(read_pin(pins.so) == HIGH);
    assert(read_pin(pins.be) == HIGH);
    assert(read_pin(pins.ready) == HIGH);
    assert(read_pin(pins.ml) == LOW);
}

void test_cpu_tick() {
    uint8_t buff[] = {0, 0, 0, 0, 0, 0, 0};
    Serial._set_read_buff(buff, 7);
}

void test_pins_to_msg() {
    uint8_t buff[5];
    get_pins_state(pin_ids, buff);

    assert(buff[0] & 128); // reset
    assert(buff[4] & 8); // IRQ
}

void integration_tests_all() {
    void (* tests[])() = { test_reset, test_cpu_tick, test_pins_to_msg };
    for (auto test: tests) _run_test(test);
}
