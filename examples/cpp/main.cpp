#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>

#include "configuration.hpp"
#include "runner.hpp"

using std::string;
using std::endl;
using std::cout;

void print_screen(const Memory& mem);


int main(int argc, char *argv[])
{
    // open COM port device
    int device = open(std::string(PORT).c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (device < 0) {
        std::cout << "Couldn't open COM port: " << PORT << std::endl;
        exit(1);
    }

    // instantiate memory emulation and load example program
    Memory mem(0xfce2, 0xff48);
    if (!mem.load_program("../rom/basic.901226-01.bin", 0xa000)) {
        std::cout <<  "Error: could not open BASIC file " << PROGRAM_FILE << std::endl;
        exit(1);
    }
    if (!mem.load_program("../rom/kernal.901227-03.bin", 0xe000)) {
        std::cout <<  "Error: could not open KERNAL file " << PROGRAM_FILE << std::endl;
        exit(1);
    }

    // execute program on connected CPU with emulated RAM
    Runner runner = Runner(device, &mem);
    runner.run();
    print_screen(mem);

    close(device);
    return 0;
}

void print_screen(const Memory& mem) {
    string char_set =
        "@abcdefghijklmnopqrstuvwxyz[£]↑← !\"#$%&'()*+,-./0123456789:;<=>?\
         -ABCDEFGHIJKLMNOPQRSTUVWXYZ[£]↑← !\"#$%&'()*+,-./0123456789:;<=>?\
         @ABCDEFGHIJKLMNOPQRSTUVWXYZ[£]↑← !\"#$%&'()*+,-./0123456789:;<=>?\
         -abcdefghijklmnopqrstuvwxyz[£]↑← !\"#$%&'()*+,-./0123456789:;<=>?";
    uint64_t n = 0;
    cout << endl;
    cout << string(44, ' ') << endl;
    cout << "  ";
    for (uint16_t i=0x0400; i<0x07e8; ++i) {
        uint8_t sc = mem.read_byte(i);
        cout << char_set[sc];
        n += 1;
        if (n % 40 == 0) {
            cout << "  " << endl << "  ";
        }
    }
    cout << string(42, ' ') << endl;
    cout << "              " << endl;
}
