#include "tests.hpp"
#include "../src/protocol.hpp"

void test_bytes_to_msg()
{
    uint8_t bytes[] = { 2, 23, 11, 0, 255, 2, 33 };
    message_t msg = create_msg_from_bytes(bytes);
    assert(msg.type == 2);
    assert(msg.size == 5);
    assert(msg.checksum == 33);
    assert(compute_checksum(&msg) == 37);
    for (int i=0; i<msg.size; ++i) {
        assert(bytes[i+1] == msg.data[i]);
    }
}

void test_msg_to_bytes()
{
    message_t msg = {
        .type = 2,
        .size = 5,
        .checksum = 10,
        .data = { 11, 0, 22, 255, 5 }
    };
    uint8_t buff[7];
    msg_to_buff(&msg, buff);

    assert(buff[0] == 2);
    for (int i=0; i<msg.size; ++i) {
        assert(buff[i+1] == msg.data[i]);
    }
    assert(buff[6] == msg.checksum);
}

void protocol_tests_all()
{
    test_bytes_to_msg();
    test_msg_to_bytes();
}
