#include <cstdio>
#include <iostream>
#include "memory.hpp"

using namespace std;

Memory::Memory(const uint16_t program_addr, const uint16_t irq_addr)
{
    this->program_addr = program_addr;

    this->write_byte(0xffff, (irq_addr & 0xff00) >> 8);
    this->write_byte(0xfffe, irq_addr & 0xff);

    this->write_byte(0xfffb, (irq_addr & 0xff00) >> 8);
    this->write_byte(0xfffa, irq_addr & 0xff);

    this->write_byte(0xfffd, (program_addr & 0xff00) >> 8);
    this->write_byte(0xfffc, program_addr & 0xff);
}

uint8_t Memory::read_byte(const uint16_t addr) const
{
    return this->data[addr];
}

void Memory::write_byte(uint16_t addr, uint8_t val)
{
    this->data[addr] = val;
}

bool Memory::load_program(std::string filename)
{
    return this->load_program(filename, this->program_addr);
}

bool Memory::load_program(std::string filename, uint16_t addr)
{
    static const int BUFFERSIZE = 1024;
    FILE* filp = fopen(filename.c_str(), "rb");
    if (!filp) {
        return false;
    }

    char * buffer = new char[BUFFERSIZE];
    int bytes;
    int cursor = addr;
    while ((bytes = fread(buffer, sizeof(char), BUFFERSIZE, filp)) > 0) {
        for (int i = 0; i < bytes; ++i) {
            this->write_byte(cursor++, (uint8_t)buffer[i]);
        }
    }

    fclose(filp);
    return true;
}

