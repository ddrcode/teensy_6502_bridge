#pragma once

#include <cstdint>
#include "pins.hpp"

void loop_debug(pins_t& pins);
void print_pin(char* label, uint8_t pin);
void print_status(pins_t &pins);
