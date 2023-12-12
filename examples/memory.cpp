#include <cstdio>
#include <iostream>
#include "memory.hpp"

using namespace std;

Memory::Memory(const uint16_t program_addr)
{
    this->program_addr = program_addr;
    this->write_byte(0xfffd, (program_addr & 0xff00) >> 8);
    this->write_byte(0xfffc, program_addr & 0xff);
}

uint8_t Memory::read_byte(uint16_t addr)
{
    return this->data[addr];
}

void Memory::write_byte(uint16_t addr, uint8_t val)
{
    this->data[addr] = val;
}

bool Memory::load_program(std::string filename)
{
    static const int BUFFERSIZE = 1024;
    FILE* filp = fopen(filename.c_str(), "rb");
    if (!filp) {
        return false;
    }

    char * buffer = new char[BUFFERSIZE];
    int bytes;
    int cursor = this->program_addr;
    while ((bytes = fread(buffer, sizeof(char), BUFFERSIZE, filp)) > 0) {
        for (int i = 0; i < bytes; ++i) {
            this->write_byte(cursor++, (uint8_t)buffer[i]);
        }
    }

    fclose(filp);
    return true;
}

