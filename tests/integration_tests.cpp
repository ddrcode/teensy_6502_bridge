#include "tests.hpp"
#include "../src/pins.hpp"
#include "../src/cpu.hpp"
#include "mocks/hardware.hpp"

uint8_t pin_ids[40];
pins_t pins;

void _setup() {
    _reset_hardware_mocks();
    Serial._reset();
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

void test_handle_cycle_sets_data_direction() {
    mocked_pins[pins.rw].val = HIGH;
    mocked_pins[pins.phi2].val = HIGH;
    handle_cycle(pins);
    for (int i = 0; i < 8; ++i) {
        assertm(mocked_pins[pins.data[i]].dir == OUTPUT, "Data bus should drive during the high phase of a read cycle");
    }

    mocked_pins[pins.phi2].val = LOW;
    handle_cycle(pins);
    for (int i = 0; i < 8; ++i) {
        assertm(mocked_pins[pins.data[i]].dir == INPUT, "Data bus must release during the low clock phase");
    }

    mocked_pins[pins.phi2].val = HIGH;
    mocked_pins[pins.rw].val = LOW;
    handle_cycle(pins);
    for (int i = 0; i < 8; ++i) {
        assertm(mocked_pins[pins.data[i]].dir == INPUT, "Data bus must release when RW is low");
    }
}

void test_set_pins_state_updates_control_lines() {
    mocked_pins[pins.rw].val = HIGH;
    uint8_t buff[BUFFSIZE] = { 0 };

    set_pins_state(pin_ids, pins, buff);
    assertm(read_pin(pins.reset) == LOW, "Reset should follow serialized state (low)");
    assertm(read_pin(pins.irq) == LOW, "IRQ should follow serialized state (low)");

    buff[0] = 0x80; // pin 39 -> reset high
    buff[4] = 0x08; // pin 3  -> IRQ high
    set_pins_state(pin_ids, pins, buff);
    assertm(read_pin(pins.reset) == HIGH, "Reset should follow serialized state (high)");
    assertm(read_pin(pins.irq) == HIGH, "IRQ should follow serialized state (high)");
}

void test_pins_to_msg() {
    mocked_pins[pins.vp].val = HIGH; // VP sits on Teensy pin 0 - a valid id
    uint8_t buff[5];
    get_pins_state(pin_ids, buff);

    assert(buff[0] & 128); // reset
    assert(buff[4] & 8); // IRQ
    assertm(buff[4] & 1, "VP (Teensy pin 0) must be reported");
}

void integration_tests_all() {
    void (* tests[])() = {
        test_reset,
        test_handle_cycle_sets_data_direction,
        test_set_pins_state_updates_control_lines,
        test_pins_to_msg
    };
    for (auto test: tests) _run_test(test);
}
