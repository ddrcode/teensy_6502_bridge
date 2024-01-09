#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>

#include "configuration.hpp"
#include "runner.hpp"

int main(int argc, char *argv[])
{
    // open COM port device
    int device = open(std::string(PORT).c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (device < 0) {
        std::cout << "Couldn't open COM port: " << PORT << std::endl;
        exit(1);
    }

    // instantiate memory emulation and load example program
    Memory mem(PROGRAM_ADDR, INTERRUPT_ADDR);
    if (!mem.load_program(PROGRAM_FILE)) {
        std::cout <<  "Error: could not open file " << PROGRAM_FILE << std::endl;
        exit(1);
    }

    // execute program on connected CPU with emulated RAM
    Runner runner = Runner(device, &mem);
    runner.run();

    close(device);
    return 0;
}
