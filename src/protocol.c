#include <stdint.h>
#include <string.h>
#include "protocol.h"

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
    auto msg = msg_error_t {
        .id = MSG_ERROR,
        .code = code,
        .checksum = MSG_ERROR + code
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
    if (t.size < 3) return true;
    let checksum = t.type;
    for (uint8_t i=t.size-2; i>1; --i) {
        checksum += t.bytes[i];
    }
    return checksum == t.bytes[t.size-1];
}
