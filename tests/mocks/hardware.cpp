#include <cassert>
#include <cstring>
#include "hardware.hpp"

mocked_pin_t mocked_pins[MOCKED_PINS_SIZE];
SerialClass Serial;

// Arduino / Teensy functions

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

// Serial class

SerialClass::SerialClass() {
    this->_reset();
}

uint8_t SerialClass::read() {
    assert(this->cursor < 10);
    return this->buff[this->cursor++];
}

void SerialClass::write(uint8_t * buff, size_t size) {
}

void SerialClass::send_now() {}

void SerialClass::_set_read_buff(uint8_t* buff, size_t size) {
    this->_reset();
    memcpy(this->buff, buff, size);
}

void SerialClass::_reset() {
    this->cursor = 0;
    for (int i = 0; i < 10; ++i) {
        this->buff[i] = 0;
    }
}

// Mock control functions

void _reset_hardware_mocks() {
    for (int i = 0; i < MOCKED_PINS_SIZE; ++i) {
        mocked_pins[i].val = LOW;
        mocked_pins[i].dir = INPUT;
    }
}
