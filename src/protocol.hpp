#pragma once

typedef struct t_msg_wrapper {
    uint8_t type;
    uint8_t size;
    uint8_t bytes[];
} msg_wrapper_t;

typedef struct t_msg_pins
{
    uint8_t id;
    uint8_t data[5];
    uint8_t checksum;
} msg_pins_t;

typedef struct t_msg_error {
    uint8_t id;
    uint8_t code;
    uint8_t checksum;
} msg_error_t;

typedef enum t_msg_type
{
    MSG_ERROR = 0,
    MSG_STATUS,
    MSG_PINS,
    MSG_INVALID = 255
} msg_type;

msg_pins_t create_pins_msg(uint8_t data[5]);
uint8_t get_msg_size(const uint8_t msg_type);
