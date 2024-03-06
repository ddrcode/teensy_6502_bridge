#pragma once

#include <stddef.h>

typedef unsigned int uint;

constexpr int LOW = 0;
constexpr int HIGH = 1;

constexpr int INPUT = 0;
constexpr int OUTPUT = 1;

void pinMode(int pin_id, int dir);
int digitalReadFast(int pin_id);
void digitalWriteFast(int pin_id, int val);

void delay(int _ms);

typedef struct t_mocked_pin {
    int dir;
    int val;
} mocked_pin_t;


class SerialClass {
    private:
        uint8_t buff[10];
        uint8_t cursor;

    public:
        uint8_t read();
        void write(uint8_t* buff, size_t size);
        void send_now();

        void _set_read_buff(uint8_t* buff, size_t size);
        void _reset();

        SerialClass();
};

extern SerialClass Serial;


constexpr int MOCKED_PINS_SIZE = 42;
extern mocked_pin_t mocked_pins[MOCKED_PINS_SIZE];

void _reset_hardware_mocks();
