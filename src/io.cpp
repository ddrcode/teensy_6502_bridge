#include <Arduino.h>
#include <_Teensy.h>
#include <cstring>
#include <cstdint>

#include "io.hpp"

inline uint8_t read_byte();
void read_bytes(uint8_t* buff, uint8_t size);

message_t read_msg() {
    uint8_t msg_type = read_byte();
    uint8_t size = get_data_size(msg_type);
    message_t msg = {
        .type = static_cast<uint8_t>(size > 0 ? msg_type : MSG_INVALID),
        .size = size
    };
    // if (size > 0) {
        read_bytes(msg.data, size);
    // }
    msg.checksum = read_byte();
    return msg;
}


inline uint8_t read_byte() {
    return Serial.read();
}

void read_bytes(uint8_t* buff, const uint8_t size) {
    for(uint8_t i=size; i--; ++buff) {
        *buff = read_byte();
    }
    // for (int i=0; i < size; ++i) {
    //     buff[i] = read_byte();
    // }
}

void write_msg(const message_t * const msg) {
    uint8_t buff[msg->size+2];
    buff[0] = msg->type;
    memcpy(buff+1, msg->data, msg->size);
    buff[6] = compute_checksum(msg);
    Serial.write(buff, msg->size+2);
    Serial.send_now();
}
