#include <iostream>
#include <iomanip>
#include <unistd.h>

#include "pins.hpp"
#include "runner.hpp"
#include "pin_utils.hpp"
#include "configuration.hpp"

using std::cout;
using std::hex;
using std::dec;
using std::endl;
using std::right;
using std::setw;
using std::setfill;

Runner::Runner(int device, Memory *mem)
{
    this->device = device;
    this->mem = mem;
    this->phase = false;
    this->cycle = 0;
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
    cout << "[" << dec << this->cycle
         << "] Addr: " << setfill('0') << setw(4) << right << hex << this->pins.addr
         << " (" << addr_to_binary_str(this->pins.addr)
         << "), Data: " << setfill('0') << setw(2) << right << hex << (uint16_t)this->pins.data
         << ", R/W: " << (this->pins.rw ? "R" : "W")
         << ", VP: " << (int)this->pins.vector_pull
         << ", SYNC " << (int)this->pins.sync
         << ", IRQ " << (int)this->pins.irq
         << ", NMI " << (int)this->pins.nmi
         << ", RES " << (int)this->pins.reset
         << ", PHI1O " << (int)this->pins.phi1o
         << ", PHI2O " << (int)this->pins.phi2o
         << endl;
}

void Runner::read_serial()
{
    uint8_t buff[5];
    for (int n=0; !n;) {
        n = read(this->device, buff, BUFFSIZE);
    }
    this->pins.set_pins(buff);
}

void Runner::write_serial()
{
    uint8_t buff[5];
    this->pins.set_buff(buff);
    auto _ = write(this->device, buff, BUFFSIZE);
}

void Runner::advance_cycles()
{
    if (this->phase) {
        ++this->cycle;
    }
    this->phase = !this->phase;
}
