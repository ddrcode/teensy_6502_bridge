#include <Arduino.h>
#include <_Teensy.h>
#include <cstring>
#include <cstdint>

#include "io.hpp"

inline uint8_t read_byte();
void read_bytes(uint8_t* buff, uint8_t size);

msg_wrapper_t read_msg() {
    uint8_t msg_type = read_byte();
    uint8_t size = get_msg_size(msg_type);
    msg_wrapper_t msg = {
        .type = static_cast<uint8_t>(size > 0 ? msg_type : MSG_INVALID),
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
