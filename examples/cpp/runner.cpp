#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include "pins.hpp"
#include "runner.hpp"
#include "pin_utils.hpp"
#include "configuration.hpp"

using std::cerr;

namespace {

constexpr uint8_t MESSAGE_TYPE_ERROR = 0;
constexpr uint8_t MESSAGE_TYPE_PINS = 2;
constexpr size_t PAYLOAD_SIZE = 5;
constexpr size_t MESSAGE_SIZE = PAYLOAD_SIZE + 2; // type + payload + checksum

uint8_t compute_checksum(uint8_t type, const uint8_t* payload)
{
    uint8_t checksum = type;
    for (size_t i = 0; i < PAYLOAD_SIZE; ++i) {
        checksum = static_cast<uint8_t>(checksum + payload[i]);
    }
    return checksum;
}

void read_exact(int fd, uint8_t* buffer, size_t length)
{
    size_t offset = 0;
    while (offset < length) {
        ssize_t bytes = read(fd, buffer + offset, length - offset);
        if (bytes > 0) {
            offset += static_cast<size_t>(bytes);
            continue;
        }
        if (bytes == 0) {
            cerr << "Bridge disconnected while reading" << std::endl;
            std::exit(1);
        }
        if (errno == EINTR) {
            continue;
        }
        cerr << "Read error: " << std::strerror(errno) << std::endl;
        std::exit(1);
    }
}

void write_exact(int fd, const uint8_t* buffer, size_t length)
{
    size_t offset = 0;
    while (offset < length) {
        ssize_t bytes = write(fd, buffer + offset, length - offset);
        if (bytes > 0) {
            offset += static_cast<size_t>(bytes);
            continue;
        }
        if (bytes == 0) {
            cerr << "Bridge disconnected while writing" << std::endl;
            std::exit(1);
        }
        if (errno == EINTR) {
            continue;
        }
        cerr << "Write error: " << std::strerror(errno) << std::endl;
        std::exit(1);
    }
}

} // namespace

Runner::Runner(int device, Memory *mem, bool log_halfcycles)
{
    this->device = device;
    this->mem = mem;
    this->phase = false;
    this->cycle = 0;
    this->log_halfcycles = log_halfcycles;
}

/**
 * CPU reset procedure. It requires to keep the CPU reset pin (40)
 * low for two cycles. After that this pin is set high.
 */
void Runner::reset()
{
    uint8_t b[] = {
        0b00001010, // BE, RW high
        0b00000000,
        0b00000000,
        0b00000000,
        0b10101000  // IRQ, NMI, VDD high
    };

    for (int i = 0; i < 4; i++) {
        this->pins = W65C02Pins(b);
        this->pins.set_overflow = true;
        this->pins.phi2 = this->phase;
        this->write_serial();
        usleep(CYCLE_DURATION);
        this->read_serial();
        this->advance_cycles();
    }

    this->pins.data = 0;
    this->pins.addr = 0;
    this->pins.reset = true;
    this->pins.ready = true;
    this->pins.set_overflow = true;
}

bool Runner::step()
{
    static uint16_t addr = 0;
    static bool write = false;

    this->pins.phi2 = this->phase;
    this->write_serial();
    usleep(CYCLE_DURATION);

    this->read_serial();
    if (!this->phase) {
        addr = this->pins.addr;
        write = this->pins.is_write();
        if (!write) {
            this->pins.data = this->mem->read_byte(addr);
        }
        if (this->log_halfcycles) {
            this->print_state();
        }
    } else {
        uint8_t data = this->pins.data;
        if (write) {
            this->mem->write_byte(addr, data);
        }
        this->print_state();
        if (EXIT_ON_BRK && data == 0 && this->pins.sync) {
            return false;
        }
    }

    this->advance_cycles();
    return MAX_CYCLES == 0 || this->cycle < MAX_CYCLES;
}

void Runner::run()
{
    this->reset();
    while (this->step());
}

void Runner::print_state()
{
    const char rw_char = this->pins.rw ? 'R' : 'W';
    const char phase_char = this->phase ? 'H' : 'L';

    std::printf(
        "Cycle=%06" PRIu64 " Half=%c Addr=$%04X Data=$%02X RW=%c SYNC=%d VP=%d IRQ=%d NMI=%d RES=%d",
        this->cycle,
        phase_char,
        static_cast<unsigned>(this->pins.addr),
        static_cast<unsigned>(this->pins.data),
        rw_char,
        static_cast<int>(this->pins.sync),
        static_cast<int>(this->pins.vector_pull),
        static_cast<int>(this->pins.irq),
        static_cast<int>(this->pins.nmi),
        static_cast<int>(this->pins.reset)
    );

    if (this->log_halfcycles) {
        std::printf(
            " PHI1O=%d PHI2O=%d",
            static_cast<int>(this->pins.phi1o),
            static_cast<int>(this->pins.phi2o)
        );
    }

    std::printf("\n");
}

void Runner::read_serial()
{
    uint8_t type;
    read_exact(this->device, &type, 1);

    if (type == MESSAGE_TYPE_ERROR) {
        uint8_t rest[2]; // error code + checksum
        read_exact(this->device, rest, 2);
        cerr << "Bridge rejected the message with error code "
             << static_cast<int>(rest[0]) << std::endl;
        std::exit(1);
    }

    if (type != MESSAGE_TYPE_PINS) {
        cerr << "Unexpected message type " << static_cast<int>(type) << std::endl;
        std::exit(1);
    }

    uint8_t message[MESSAGE_SIZE];
    message[0] = type;
    read_exact(this->device, message + 1, MESSAGE_SIZE - 1);

    const uint8_t checksum = compute_checksum(message[0], message + 1);
    if (checksum != message[MESSAGE_SIZE - 1]) {
           cerr << "Checksum mismatch: expected " << static_cast<int>(checksum)
               << " got " << static_cast<int>(message[MESSAGE_SIZE - 1]) << std::endl;
        std::exit(1);
    }

    this->pins.set_pins(message + 1);
}

void Runner::write_serial()
{
    uint8_t payload[PAYLOAD_SIZE] = { 0 };
    this->pins.set_buff(payload);

    uint8_t message[MESSAGE_SIZE];
    message[0] = MESSAGE_TYPE_PINS;
    std::memcpy(message + 1, payload, PAYLOAD_SIZE);
    message[MESSAGE_SIZE - 1] = compute_checksum(message[0], payload);

    write_exact(this->device, message, MESSAGE_SIZE);
}

void Runner::advance_cycles()
{
    if (this->phase) {
        ++this->cycle;
    }
    this->phase = !this->phase;
}
