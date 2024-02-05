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

bool validate_checksum(const msg_wrapper_t msg) {
    if (msg.size < 3) return true;
    uint8_t checksum = msg.type;
    for (uint8_t i=msg.size-2; i>1; --i) {
        checksum += msg.bytes[i];
    }
    return checksum == msg.bytes[msg.size-1];
}
