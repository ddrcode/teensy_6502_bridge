#pragma once

#include <cstdint>
#include <string>

std::string pins_to_string(uint8_t pins[]);
uint8_t get_pin(uint8_t pins[], uint8_t pin);
void set_pin(uint8_t pins[], uint8_t pin, bool value);
uint16_t decode_addr(uint8_t pins[]);
uint8_t decode_data(uint8_t pins[]);
void set_data(uint8_t pins[], uint8_t data);
std::string addr_to_binary_str(uint16_t addr);

