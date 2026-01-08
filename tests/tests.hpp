#pragma once

#include <cassert>

#define assertm(exp, msg) assert(((void)msg, exp))

void protocol_tests_all();
void integration_tests_all();

