#include <cstdint>
#include <string>

class Memory
{
private:
    uint8_t data[1<<16];
    uint16_t program_addr;

public:
    Memory(const uint16_t program_addr, const uint16_t irq_addr);
    uint8_t read_byte(uint16_t addr);
    void write_byte(uint16_t addr, uint8_t val);
    bool load_program(std::string filename);
};
