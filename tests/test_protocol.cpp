#include <cstdint>
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

void test_create_pins_msg()
{
    uint8_t data[5] = { 1, 2, 3, 4, 5 };
    msg_pins_t msg = create_pins_msg(data);
    assert(msg.id == MSG_PINS);
    uint8_t expected_checksum = MSG_PINS;
    for (int i = 0; i < 5; ++i) {
        assert(msg.data[i] == data[i]);
        expected_checksum = static_cast<uint8_t>(expected_checksum + data[i]);
    }
    assert(msg.checksum == expected_checksum);
}

void test_create_error_msg()
{
    constexpr uint8_t error_code = 0x42;
    msg_error_t msg = create_error_msg(error_code);
    assert(msg.id == MSG_ERROR);
    assert(msg.code == error_code);
    assert(msg.checksum == static_cast<uint8_t>(MSG_ERROR + error_code));
}

void test_validate_checksum()
{
    message_t msg = {
        .type = MSG_PINS,
        .size = 5,
        .checksum = 0,
        .data = { 1, 2, 3, 4, 5 }
    };
    msg.checksum = compute_checksum(&msg);
    assert(validate_checksum(&msg));
    msg.checksum = static_cast<uint8_t>(msg.checksum + 1);
    assert(!validate_checksum(&msg));
}

void test_get_msg_size()
{
    assert(get_msg_size(MSG_ERROR) == 3);
    assert(get_msg_size(MSG_STATUS) == 3);
    assert(get_msg_size(MSG_PINS) == 7);
    assert(get_msg_size(MSG_INVALID) == 0);

    assert(get_data_size(MSG_ERROR) == 1);
    assert(get_data_size(MSG_STATUS) == 1);
    assert(get_data_size(MSG_PINS) == 5);
    assert(get_data_size(MSG_INVALID) == 0);
}

void protocol_tests_all()
{
    test_bytes_to_msg();
    test_msg_to_bytes();
    test_create_pins_msg();
    test_create_error_msg();
    test_validate_checksum();
    test_get_msg_size();
}
