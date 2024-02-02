#pragma once

typedef struct t_msg_pins {
    uint8_t id;
    uint8_t data[5];
    uint8_t checksum;
} msg_pins_t;


typedef enum t_msg_type {
    MSG_ERROR = 0,
    MSG_STATUS,
    MSG_PINS
} msg_type;

msg_pins_t create_pins_msg(uint8_t data[5]);
