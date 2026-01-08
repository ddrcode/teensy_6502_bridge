#pragma once

#include "protocol.hpp"


message_t read_msg();
void write_msg(const message_t * const msg);

