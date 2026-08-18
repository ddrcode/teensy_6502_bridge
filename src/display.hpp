#pragma once

#include <cstdint>

// One captured half-cycle, decoded from the 5-byte pins payload.
typedef struct t_display_sample {
    uint16_t addr;
    uint8_t data;
    uint8_t flags;
} display_sample_t;

// Bit masks for display_sample_t.flags (raw pin levels, not assertions)
constexpr uint8_t DISP_PHI2 = 1 << 0;
constexpr uint8_t DISP_SYNC = 1 << 1;
constexpr uint8_t DISP_RW   = 1 << 2;
constexpr uint8_t DISP_VP   = 1 << 3;
constexpr uint8_t DISP_ML   = 1 << 4;
constexpr uint8_t DISP_IRQ  = 1 << 5;
constexpr uint8_t DISP_NMI  = 1 << 6;
constexpr uint8_t DISP_RES  = 1 << 7;

// Decodes a wire-format pins payload (see docs/protocol.md) into a sample.
// Pure logic - covered by unit tests.
display_sample_t decode_sample(const uint8_t payload[5]);

// The functions below are real only when ENABLE_DISPLAY is defined
// (see configuration.h); the .ino guards all call sites.
void display_setup();
void display_capture(const uint8_t payload[5]);
void display_refresh();
void display_idle();
