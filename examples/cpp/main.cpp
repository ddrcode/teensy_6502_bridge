#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <termios.h>
#include <sys/ioctl.h>

#include "configuration.hpp"
#include "runner.hpp"

namespace {

void configure_serial_port(int fd)
{
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "tcgetattr failed: " << std::strerror(errno) << std::endl;
        exit(1);
    }

    cfmakeraw(&tty);
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "tcsetattr failed: " << std::strerror(errno) << std::endl;
        exit(1);
    }

    int modem_bits = TIOCM_DTR | TIOCM_RTS;
    if (ioctl(fd, TIOCMBIS, &modem_bits) == -1) {
        std::cerr << "ioctl TIOCMBIS failed: " << std::strerror(errno) << std::endl;
        exit(1);
    }
}

} // namespace

int main(int argc, char *argv[])
{
    // open COM port device
    int device = open(std::string(PORT).c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (device < 0) {
        std::cout << "Couldn't open COM port: " << PORT << std::endl;
        exit(1);
    }

    configure_serial_port(device);

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
