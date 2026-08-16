#include <cstdint>
#include "tests.hpp"
#include "../src/io.hpp"
#include "mocks/hardware.hpp"

void test_read_from_empty_port()
{
    Serial._reset();
    message_t msg;
    assertm(!try_read_msg(&msg), "Nothing to read from an empty port");
}

void test_read_incomplete_msg()
{
    uint8_t bytes[] = { 2, 1, 2, 3 }; // a pins message needs 7 bytes
    Serial._set_read_buff(bytes, 4);
    message_t msg;
    assertm(!try_read_msg(&msg), "Incomplete message must not be reported as ready");
    assertm(Serial.available() == 4, "Incomplete message must not be consumed");
}

void test_read_complete_msg()
{
    uint8_t bytes[] = { 2, 1, 2, 3, 4, 5, 17 };
    Serial._set_read_buff(bytes, 7);
    message_t msg;
    assert(try_read_msg(&msg));
    assert(msg.type == MSG_PINS);
    assert(msg.size == 5);
    assert(msg.checksum == 17);
    assert(validate_checksum(&msg));
    for (int i = 0; i < msg.size; ++i) {
        assert(msg.data[i] == bytes[i + 1]);
    }
    assert(Serial.available() == 0);
}

void test_read_unknown_msg_type()
{
    uint8_t bytes[] = { 9, 1, 2 };
    Serial._set_read_buff(bytes, 3);
    message_t msg;
    assert(try_read_msg(&msg));
    assert(msg.type == MSG_INVALID);
    assertm(Serial.available() == 2, "Only the invalid type byte should be consumed");
}

void test_send_error()
{
    Serial._reset();
    send_error(ERR_INVALID_CHECKSUM);
    assertm(Serial._get_out_size() == 3, "Error message is 3 bytes long");
    const uint8_t* out = Serial._get_out_buff();
    assert(out[0] == MSG_ERROR);
    assert(out[1] == ERR_INVALID_CHECKSUM);
    assert(out[2] == static_cast<uint8_t>(MSG_ERROR + ERR_INVALID_CHECKSUM));
}

void io_tests_all()
{
    void (* tests[])() = {
        test_read_from_empty_port,
        test_read_incomplete_msg,
        test_read_complete_msg,
        test_read_unknown_msg_type,
        test_send_error
    };
    for (auto test: tests) {
        Serial._reset();
        test();
    }
    Serial._reset();
}
