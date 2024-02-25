#pragma once

typedef struct t_message {
    uint8_t type;
    uint8_t size;
    uint8_t checksum;
    uint8_t data[5];
} message_t;

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
uint8_t get_data_size(const uint8_t msg_type);
uint8_t compute_checksum(const message_t * const msg);
message_t create_msg_from_bytes(const uint8_t * const bytes);
void msg_to_buff(const message_t * const msg, uint8_t buff[]);
