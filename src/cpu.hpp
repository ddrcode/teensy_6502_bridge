#pragma once

#include "pins.hpp"

void setup_cpu(pins_t& pins);
void reset(pins_t& pins);
void handle_cycle(pins_t &pins);
