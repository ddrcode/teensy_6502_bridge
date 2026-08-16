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

struct CliOptions {
    std::string port;
    std::string program;
    bool show_halfcycles = false;
    bool step_mode = false;
};

[[noreturn]] void print_usage(const char* prog)
{
    std::cerr << "Usage: " << prog << " --port <device> --program <file> [--halfcycles] [--step]" << std::endl;
    std::cerr << "  --halfcycles    Log every half-cycle and include PHI pins" << std::endl;
    std::cerr << "  --step          Step mode: pause after every cycle and wait for input" << std::endl;
    exit(1);
}

/**
 * Configure /dev/ttyACM* for Teensy bridge usage.
 *
 * - Switch to raw 8N1 so no newline/echo mangling occurs
 * - Disable hardware flow control because CDC ignores RTS/CTS
 * - Block until at least one byte arrives (Runner expects read_exact)
 * - Assert DTR/RTS so Serial.dtr() is true on the Teensy side
 */
void configure_serial_port(int fd)
{
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "tcgetattr failed: " << std::strerror(errno) << std::endl;
        exit(1);
    }

    cfmakeraw(&tty);             // raw 8-N-1
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag |= (CLOCAL | CREAD); // ignore modem ctrl, enable rx
    tty.c_cflag &= ~CRTSCTS;     // no HW flow control on Teensy CDC
    tty.c_cc[VMIN] = 1;          // block until 1 byte arrives
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

CliOptions parse_cli(int argc, char* argv[])
{
    CliOptions options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto require_value = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << name << " requires a value" << std::endl;
                print_usage(argv[0]);
            }
            return argv[++i];
        };

        if (arg == "-p" || arg == "--port") {
            options.port = require_value(arg);
        } else if (arg == "-f" || arg == "--program") {
            options.program = require_value(arg);
        } else if (arg == "--halfcycles") {
            options.show_halfcycles = true;
        } else if (arg == "--step") {
            options.step_mode = true;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_usage(argv[0]);
        }
    }

    if (options.port.empty() || options.program.empty()) {
        std::cerr << "Port and program path are required" << std::endl;
        print_usage(argv[0]);
    }

    return options;
}

} // namespace

int main(int argc, char *argv[])
{
    CliOptions options = parse_cli(argc, argv);

    // open COM port device
    int device = open(options.port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (device < 0) {
        std::cerr << "Couldn't open COM port: " << options.port << std::endl;
        exit(1);
    }

    configure_serial_port(device);

    // instantiate memory emulation and load example program
    Memory mem(PROGRAM_ADDR, INTERRUPT_ADDR);
    if (!mem.load_program(options.program)) {
        std::cout <<  "Error: could not open file " << options.program << std::endl;
        exit(1);
    }

    // execute program on connected CPU with emulated RAM
    Runner runner = Runner(device, &mem, options.show_halfcycles, options.step_mode);
    runner.run();

    close(device);
    return 0;
}
