#include <cstdint>
#include <cstring>
#include "tests.hpp"
#include "../src/display.hpp"

namespace {

// Sets bit N of the 40-bit payload (bit N = CPU pin N+1, big-endian bytes)
void set_bit(uint8_t payload[5], uint8_t bit)
{
    payload[4 - (bit >> 3)] |= 1 << (bit & 7);
}

void encode_addr(uint8_t payload[5], uint16_t addr)
{
    for (int i = 0; i < 12; ++i) {
        if (addr & (1 << i)) set_bit(payload, 8 + i);
    }
    for (int i = 0; i < 4; ++i) {
        if (addr & (1 << (12 + i))) set_bit(payload, 21 + i);
    }
}

void encode_data(uint8_t payload[5], uint8_t data)
{
    for (int i = 0; i < 8; ++i) {
        if (data & (1 << (7 - i))) set_bit(payload, 25 + i);
    }
}

} // namespace

void test_decode_addr_and_data()
{
    uint8_t payload[5] = { 0 };
    encode_addr(payload, 0xABCD);
    encode_data(payload, 0xB1);

    display_sample_t s = decode_sample(payload);
    assert(s.addr == 0xABCD);
    assert(s.data == 0xB1);
    assert(s.flags == 0);
}

void test_decode_flags()
{
    uint8_t payload[5] = { 0 };
    set_bit(payload, 36); // PHI2
    set_bit(payload, 6);  // SYNC
    set_bit(payload, 33); // RW
    set_bit(payload, 0);  // VP
    set_bit(payload, 4);  // ML
    set_bit(payload, 3);  // IRQ
    set_bit(payload, 5);  // NMI
    set_bit(payload, 39); // RES

    display_sample_t s = decode_sample(payload);
    assert(s.flags == (DISP_PHI2 | DISP_SYNC | DISP_RW | DISP_VP |
                       DISP_ML | DISP_IRQ | DISP_NMI | DISP_RES));
    assert(s.addr == 0);
    assert(s.data == 0);
}

void test_decode_real_capture()
{
    // Payload of an actual bridge reply captured on hardware (during reset,
    // low phase): addr=$0239, data=$00, RW high, IRQ/NMI/ML high,
    // VP/SYNC/PHI2/RES low.
    uint8_t payload[5] = { 0x0A, 0x00, 0x02, 0x39, 0x3E };

    display_sample_t s = decode_sample(payload);
    assert(s.addr == 0x0239);
    assert(s.data == 0x00);
    assert(s.flags & DISP_RW);
    assert(s.flags & DISP_IRQ);
    assert(s.flags & DISP_NMI);
    assert(s.flags & DISP_ML);
    assert(!(s.flags & DISP_VP));
    assert(!(s.flags & DISP_SYNC));
    assert(!(s.flags & DISP_PHI2));
    assert(!(s.flags & DISP_RES));
}

void display_tests_all()
{
    test_decode_addr_and_data();
    test_decode_flags();
    test_decode_real_capture();
}
