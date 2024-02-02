#include <stdint.h>
#include <string.h>
#include "protocol.h"

msg_pins_t create_pins_msg(uint8_t data[5]) {
    uint8_t checksum = MSG_PINS;
    for (int i=0; i<5; ++i) checksum += data[i];
    msg_pins_t msg = {
        .id = MSG_PINS,
        .checksum = checksum
    };
    memcpy(msg.data, data, 5);
    return msg;
}
