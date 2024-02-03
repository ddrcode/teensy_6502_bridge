#pragma once

#include "pins.h"

void setup_cpu(pins_t& pins);
void reset();
void handle_cycle(pins_t &pins);
