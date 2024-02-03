#pragma once

#include <stdint.h>
#include "pins.h"

void loop_debug();
void print_pin(char* label, uint8_t pin);
void print_status(pins_t &pins);
