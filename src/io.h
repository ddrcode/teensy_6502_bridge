#pragma once

inline bool is_connected();
inline bool is_available();
inline uint8_t* read_message();
inline void send_message(uint8_t* msg, size_t size);
