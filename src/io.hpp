#pragma once

#include "protocol.hpp"


bool try_read_msg(message_t* msg);
void send_error(const uint8_t code);
