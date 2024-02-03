#include "io.h"

msg_wrapper_t read_msg() {
    int8_t msg_type = read_byte();
    int8_t size = get_msg_size(msg_type);
    msg_wrapper msg = {
        .type = size > 0 ? msg_type : MSG_INVALID,
        .size = size
    };
    if (size > 1) {
        uint8_t buff[size];
        read_bytes(buff, size);
        memcpy(msg.bytes, buff, size-1);
    }
    return msg;
}


inline uint8_t read_byte() {
    return Serial.read();
}

void read_bytes(uint8_t* buff, uint8_t size) {
    for(uint8_t i=size; i--; ++buff) {
        *buff = read_byte();
    }
}
