#include <cstdint>

#include "hardware.hpp"
#include "io.hpp"

inline uint8_t read_byte();
void read_bytes(uint8_t* buff, uint8_t size);

/**
 * Attempts to read a complete message from the serial port without blocking.
 * Returns false when a full message hasn't arrived yet (call again later);
 * in that case nothing is consumed from the port.
 * Returns true when msg has been populated. An unrecognized type byte is
 * consumed and reported as a message of type MSG_INVALID (with no payload).
 */
bool try_read_msg(message_t* msg) {
    if (!Serial.available()) {
        return false;
    }

    const uint8_t type = static_cast<uint8_t>(Serial.peek());
    const uint8_t msg_size = get_msg_size(type);

    if (msg_size == 0) { // unknown type - consume the byte and report
        Serial.read();
        msg->type = MSG_INVALID;
        msg->size = 0;
        msg->checksum = 0;
        return true;
    }

    if (Serial.available() < msg_size) { // wait for the complete message
        return false;
    }

    msg->type = read_byte();
    msg->size = get_data_size(type);
    read_bytes(msg->data, msg->size);
    msg->checksum = read_byte();
    return true;
}

void send_error(const uint8_t code) {
    msg_error_t msg = create_error_msg(code);
    uint8_t buff[3] = { msg.id, msg.code, msg.checksum };
    Serial.write(buff, 3);
    Serial.send_now();
}

inline uint8_t read_byte() {
    return Serial.read();
}

void read_bytes(uint8_t* buff, const uint8_t size) {
    for(uint8_t i=size; i--; ++buff) {
        *buff = read_byte();
    }
}
