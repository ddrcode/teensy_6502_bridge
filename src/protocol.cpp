#include <cstdint>
#include <cstring>
#include "protocol.hpp"

msg_pins_t create_pins_msg(uint8_t data[5])
{
    uint8_t checksum = MSG_PINS;
    for (int i=0; i<5; ++i) checksum += data[i];
    msg_pins_t msg = {
        .id = MSG_PINS,
        .checksum = checksum
    };
    memcpy(msg.data, data, 5);
    return msg;
}

msg_error_t create_error_msg(uint8_t code) {
    msg_error_t msg = {
        .id = MSG_ERROR,
        .code = code,
        .checksum = static_cast<uint8_t>(MSG_ERROR + code)
    };
    return msg;
}

uint8_t get_msg_size(const uint8_t msg_type) {
    switch(msg_type) {
        case MSG_ERROR:
        case MSG_STATUS: return 3;
        case MSG_PINS: return 7;
        default: return 0;
    }
}

uint8_t get_data_size(const uint8_t msg_type) {
    const uint8_t msg_size = get_msg_size(msg_type);
    return msg_size < 2 ? msg_size : msg_size - 2;
}

uint8_t compute_checksum(const message_t * const msg) {
    if (msg->size == 0) return 0;
    uint8_t checksum = msg->type;
    for (uint8_t i=0; i < msg->size; ++i) {
        checksum += msg->data[i];
    }
    return checksum;
}

bool validate_checksum(const message_t * const msg) {
    return msg->checksum == compute_checksum(msg);
}

message_t create_msg_from_bytes(const uint8_t * const bytes) {
    uint8_t type = bytes[0];
    uint8_t size = get_data_size(type);
    message_t msg = {
        .type = type,
        .size = size,
        .checksum = bytes[size+1],
    };
    memcpy(msg.data, bytes+1, size);
    return msg;
}

void msg_to_buff(const message_t * const msg, uint8_t buff[]) {
    buff[0] = msg->type;
    memcpy(buff+1, msg->data, msg->size);
    buff[msg->size + 1] = msg->checksum;
}
