#include <iostream>
#include "tests.hpp"

int main() {
    protocol_tests_all();
    integration_tests_all();
    std::cout << "All OK" << std::endl;
    return 0;
}
