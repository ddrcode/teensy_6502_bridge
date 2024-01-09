#include <bitset>
#include "pin_utils.hpp"

using namespace std;

uint16_t decode_addr(uint8_t pins[])
{
    static const uint8_t addr_pins[16] = {
        8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 24
    };
    uint16_t addr = 0;
    for (int i = 0; i < 16; ++i) {
        addr |= get_pin(pins, addr_pins[i]) << i;
    }
    return addr;
}

uint8_t decode_data(uint8_t pins[])
{
    uint8_t data = 0;
    for (int i = 0; i < 8; ++i) {
        data |= get_pin(pins, 32-i) << i;
    }
    return data;
}

void set_data(uint8_t pins[], uint8_t data)
{
    int pin_id = 32;
    for (int i = 0; i < 8; ++i) {
        set_pin(pins, pin_id--, data & (1 << i));
    }
}

uint8_t get_pin(uint8_t pins[], uint8_t pin)
{
    int cell = (39-pin) / 8;
    return pins[cell] & (1 << (pin % 8)) ? 1 : 0;
}

void set_pin(uint8_t pins[], uint8_t pin, bool value)
{
    int cell = (39-pin) / 8;
    pins[cell] ^= value << (pin % 8);
    pins[cell] ^= 1 << (pin % 8);
}

string pins_to_string(uint8_t pins[])
{
    string result;
    for (int i = 4; i >= 0; --i) {
        result += bitset<8>(pins[i]).to_string();
        result += " ";
    }
    return result;
}

string addr_to_binary_str(uint16_t addr)
{
    return bitset<16>(addr).to_string();
}
