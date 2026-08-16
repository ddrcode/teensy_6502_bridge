#include <cassert>
#include <cstring>
#include <cstdint>
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
    assert(this->cursor < this->size);
    return this->buff[this->cursor++];
}

int SerialClass::peek() {
    return this->cursor < this->size ? this->buff[this->cursor] : -1;
}

int SerialClass::available() {
    return this->size - this->cursor;
}

void SerialClass::write(uint8_t * buff, size_t size) {
    assert(this->out_size + size <= SERIAL_MOCK_OUT_SIZE);
    memcpy(this->out_buff + this->out_size, buff, size);
    this->out_size += static_cast<uint8_t>(size);
}

void SerialClass::send_now() {}

void SerialClass::_set_read_buff(uint8_t* buff, size_t size) {
    this->_reset();
    assert(size <= SERIAL_MOCK_BUFF_SIZE);
    memcpy(this->buff, buff, size);
    this->size = static_cast<uint8_t>(size);
}

void SerialClass::_reset() {
    this->cursor = 0;
    this->size = 0;
    this->out_size = 0;
    memset(this->buff, 0, SERIAL_MOCK_BUFF_SIZE);
    memset(this->out_buff, 0, SERIAL_MOCK_OUT_SIZE);
}

uint8_t SerialClass::_get_out_size() {
    return this->out_size;
}

const uint8_t* SerialClass::_get_out_buff() {
    return this->out_buff;
}

// Mock control functions

void _reset_hardware_mocks() {
    for (int i = 0; i < MOCKED_PINS_SIZE; ++i) {
        mocked_pins[i].val = LOW;
        mocked_pins[i].dir = INPUT;
    }
}
