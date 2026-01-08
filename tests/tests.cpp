#include <iostream>
#include "tests.hpp"
#include "mocks/hardware.hpp"

int main() {
    _reset_hardware_mocks();
    protocol_tests_all();
    integration_tests_all();
    std::cout << "All OK" << std::endl;
    return 0;
}
