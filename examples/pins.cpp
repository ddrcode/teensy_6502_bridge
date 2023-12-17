#include "pins.hpp"
#include "pin_utils.hpp"

W65C02Pins::W65C02Pins()
{
    this->set_pins(new uint8_t[5] {0,0,0,0,0});
}

W65C02Pins::W65C02Pins(uint8_t pins[5])
{
    this->set_pins(pins);
}

bool W65C02Pins::is_write()
{
    return !this->rw; // write is on low
}

void W65C02Pins::set_pins(uint8_t pins[5])
{
    this->vector_pull = pin(pins, 0);
    this->ready = pin(pins, 1);
    this->phi1o = pin(pins, 2);
    this->irq = pin(pins, 3);
    this->memory_lock = pin(pins, 4);
    this->nmi = pin(pins, 5);
    this->sync = pin(pins, 6);

    this->addr = decode_addr(pins);
    this->data = decode_data(pins);

    this->rw = pin(pins, 33);
    this->bus_enable = pin(pins, 35);
    this->phi2 = pin(pins, 36);
    this->set_overflow = pin(pins, 37);
    this->phi2o = pin(pins, 38);
    this->reset = pin(pins, 39);
}

void W65C02Pins::set_buff(uint8_t buff[5])
{
    set_pin(buff, 0, this->vector_pull);
    set_pin(buff, 1, this->ready);
    set_pin(buff, 2, this->phi1o);
    set_pin(buff, 3, this->irq);
    set_pin(buff, 4, this->memory_lock);
    set_pin(buff, 5, this->nmi);
    set_pin(buff, 6, this->sync);

    set_data(buff, this->data);

    set_pin(buff, 33, this->rw);
    set_pin(buff, 35, this->bus_enable);
    set_pin(buff, 36, this->phi2);
    set_pin(buff, 37, this->set_overflow);
    set_pin(buff, 38, this->phi2o);
    set_pin(buff, 39, this->reset);
}
