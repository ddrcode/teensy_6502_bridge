#pragma once

#include <stddef.h>
#include <cstdint>

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


constexpr size_t SERIAL_MOCK_BUFF_SIZE = 10;
constexpr size_t SERIAL_MOCK_OUT_SIZE = 16;

class SerialClass {
    private:
        uint8_t buff[SERIAL_MOCK_BUFF_SIZE];
        uint8_t cursor;
        uint8_t size;
        uint8_t out_buff[SERIAL_MOCK_OUT_SIZE];
        uint8_t out_size;

    public:
        uint8_t read();
        int peek();
        int available();
        void write(uint8_t* buff, size_t size);
        void send_now();

        void _set_read_buff(uint8_t* buff, size_t size);
        void _reset();
        uint8_t _get_out_size();
        const uint8_t* _get_out_buff();

        SerialClass();
};

extern SerialClass Serial;


constexpr int MOCKED_PINS_SIZE = 42;
extern mocked_pin_t mocked_pins[MOCKED_PINS_SIZE];

void _reset_hardware_mocks();
