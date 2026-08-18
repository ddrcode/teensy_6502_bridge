/*
   Optional ILI9341 status display ("front panel") for the bridge.

   Matches the PCB v1 LCD header: the screen sits on SPI1 (MOSI=26, SCK=27,
   MISO=1) with D/C on pin 10; CS is tied to ground and RST is pulled high
   on the board, so neither needs a GPIO.

   The panel shows a header with the current address/data and CPU status
   flags, a sweeping 8-lane logic analyzer fed from the same 5-byte pin
   payload the serial protocol uses (one column per half-cycle), and a
   footer with the last opcode fetch, the effective clock rate and a cycle
   counter. Before a host connects, a splash screen is shown instead.

   Author: ddrcode
   Repository: https://github.com/ddrcode/teensy_6502_bridge/
   The MIT License
*/

#include <cstdint>

#include "../configuration.h"
#include "display.hpp"

//--------------------------------------------------------------------------
// Payload decoding (pure logic, unit-tested)

// Wire format: bit N of the 40-bit payload = CPU pin N+1;
// payload byte 0 holds bits 39..32 (big-endian), see docs/protocol.md.
static inline bool payload_bit(const uint8_t payload[5], const uint8_t bit)
{
    return payload[4 - (bit >> 3)] & (1 << (bit & 7));
}

display_sample_t decode_sample(const uint8_t payload[5])
{
    display_sample_t s = { 0, 0, 0 };

    for (int i = 0; i < 12; ++i) { // A0-A11 at bits 8-19
        if (payload_bit(payload, 8 + i)) s.addr |= 1 << i;
    }
    for (int i = 0; i < 4; ++i) { // A12-A15 at bits 21-24 (bit 20 is VSS)
        if (payload_bit(payload, 21 + i)) s.addr |= 1 << (12 + i);
    }
    for (int i = 0; i < 8; ++i) { // D7-D0 at bits 25-32 (reversed)
        if (payload_bit(payload, 25 + i)) s.data |= 1 << (7 - i);
    }

    if (payload_bit(payload, 36)) s.flags |= DISP_PHI2;
    if (payload_bit(payload, 6))  s.flags |= DISP_SYNC;
    if (payload_bit(payload, 33)) s.flags |= DISP_RW;
    if (payload_bit(payload, 0))  s.flags |= DISP_VP;
    if (payload_bit(payload, 4))  s.flags |= DISP_ML;
    if (payload_bit(payload, 3))  s.flags |= DISP_IRQ;
    if (payload_bit(payload, 5))  s.flags |= DISP_NMI;
    if (payload_bit(payload, 39)) s.flags |= DISP_RES;

    return s;
}

//--------------------------------------------------------------------------
// Hardware part

#if defined(ENABLE_DISPLAY) && !defined(RUNNING_TESTS)

#include <cstdio>

#include "hardware.hpp"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Colors (matching the x16-math debugger palette)
constexpr uint16_t COL_BG     = ILI9341_BLACK;
constexpr uint16_t COL_LABEL  = 0x630C; // dark gray
constexpr uint16_t COL_VALUE  = 0xFEA0; // amber
constexpr uint16_t COL_PHI2   = ILI9341_CYAN;
constexpr uint16_t COL_SYNC   = ILI9341_MAGENTA;
constexpr uint16_t COL_RW     = ILI9341_YELLOW;
constexpr uint16_t COL_VP     = ILI9341_GREEN;
constexpr uint16_t COL_ML     = 0xFC00; // orange
constexpr uint16_t COL_IRQ    = 0x867F; // light blue
constexpr uint16_t COL_NMI    = 0x3D9F; // blue
constexpr uint16_t COL_RES    = ILI9341_RED;
constexpr uint16_t COL_READ   = ILI9341_GREEN;
constexpr uint16_t COL_WRITE  = ILI9341_RED;
constexpr uint16_t COL_CURSOR = ILI9341_WHITE;

// Layout (320x240 landscape)
constexpr int16_t HDR_Y       = 4;
constexpr int16_t FLAGS_Y     = 52;
constexpr int16_t WAVE_Y      = 72;
constexpr int16_t LANE_H      = 17;
constexpr int16_t LANE_COUNT  = 8;
constexpr int16_t WAVE_X      = 40;
constexpr int16_t COL_W       = 2;
constexpr int16_t WAVE_COLS   = 138;
constexpr int16_t FOOT_Y      = WAVE_Y + LANE_COUNT * LANE_H + 8; // 216
constexpr uint8_t MAX_COLS_PER_REFRESH = 32;

typedef struct t_lane {
    const char* name;
    uint8_t mask;
    uint16_t color;
} lane_t;

static const lane_t LANES[LANE_COUNT] = {
    { "PHI2", DISP_PHI2, COL_PHI2 },
    { "SYNC", DISP_SYNC, COL_SYNC },
    { "RW",   DISP_RW,   COL_RW   },
    { "VP",   DISP_VP,   COL_VP   },
    { "ML",   DISP_ML,   COL_ML   },
    { "IRQ",  DISP_IRQ,  COL_IRQ  },
    { "NMI",  DISP_NMI,  COL_NMI  },
    { "RES",  DISP_RES,  COL_RES  },
};

// Active-low signals are shown "lit" in the flags row when asserted (low)
constexpr uint8_t ACTIVE_LOW = DISP_VP | DISP_ML | DISP_IRQ | DISP_NMI | DISP_RES;

static Adafruit_ILI9341 tft(&SPI1, DISPLAY_PIN_DC, DISPLAY_PIN_CS, DISPLAY_PIN_RST);

// Sample ring buffer; uint8_t indices wrap naturally at 256
static display_sample_t ring[256];
static uint8_t ring_head = 0;
static uint8_t ring_tail = 0;

static bool chrome_drawn = false;   // run-screen background/labels drawn
static bool splash_drawn = false;
static int16_t sweep_col = 0;
static uint8_t prev_levels = 0;     // lane levels of the previously drawn column
static bool have_prev = false;

static uint32_t cycle_count = 0;
static bool last_phi2 = false;
static uint16_t captures = 0;       // captures since the last rate update
static uint32_t rate = 0;           // effective clock in Hz
static uint32_t last_rate_ms = 0;
static uint32_t last_refresh_ms = 0;
static uint32_t last_header_ms = 0;

// Last drawn header/footer values (to redraw only on change)
static display_sample_t shown = { 0xFFFF, 0xFF, 0xFF };
static uint16_t fetch_addr = 0;
static uint8_t fetch_data = 0;
static bool have_fetch = false;
static uint32_t shown_fetch = 0xFFFFFFFF; // addr<<8 | data, sentinel = never drawn
static uint32_t shown_rate = 1;
static uint32_t shown_cycles = 1;

//--------------------------------------------------------------------------
// Splash screen

static void draw_cat(int16_t x, int16_t y, uint16_t body, uint16_t eyes)
{
    tft.fillTriangle(x + 12, y + 16, x + 16, y - 2, x + 26, y + 12, body); // ears
    tft.fillTriangle(x + 30, y + 12, x + 40, y - 2, x + 44, y + 16, body);
    tft.fillCircle(x + 28, y + 24, 16, body);                              // head
    tft.fillRoundRect(x + 8, y + 32, 52, 44, 16, body);                    // body
    tft.fillRoundRect(x + 60, y + 62, 24, 7, 3, body);                     // tail
    tft.fillRoundRect(x + 77, y + 40, 7, 26, 3, body);
    tft.fillCircle(x + 22, y + 22, 2, eyes);                               // eyes
    tft.fillCircle(x + 34, y + 22, 2, eyes);
}

void display_idle()
{
    if (splash_drawn) {
        return;
    }
    splash_drawn = true;
    chrome_drawn = false;
    ring_tail = ring_head; // drop samples from the previous session

    tft.fillScreen(COL_BG);
    tft.setTextColor(COL_VALUE);
    tft.setTextSize(3);
    tft.setCursor(24, 60);
    tft.print("W65C02 BRIDGE");
    tft.setTextSize(1);
    tft.setTextColor(COL_LABEL);
    tft.setCursor(24, 92);
    tft.print("waiting for host...");
    draw_cat(216, 130, COL_LABEL, COL_VALUE);
}

//--------------------------------------------------------------------------
// Run screen

static void draw_chrome()
{
    tft.fillScreen(COL_BG);
    tft.setTextSize(1);
    tft.setTextColor(COL_LABEL);
    tft.setCursor(8, HDR_Y);
    tft.print("ADDR");
    tft.setCursor(140, HDR_Y);
    tft.print("DATA");
    tft.setCursor(252, HDR_Y);
    tft.print("BUS");

    for (int i = 0; i < LANE_COUNT; ++i) {
        tft.setTextColor(LANES[i].color);
        tft.setCursor(4, WAVE_Y + i * LANE_H + 4);
        tft.print(LANES[i].name);
    }

    tft.drawFastHLine(0, FLAGS_Y - 6, 320, COL_LABEL);
    tft.drawFastHLine(0, WAVE_Y - 4, 320, COL_LABEL);
    tft.drawFastHLine(0, FOOT_Y - 4, 320, COL_LABEL);

    tft.setTextColor(COL_LABEL);
    tft.setCursor(8, FOOT_Y);
    tft.print("SYNC");
    tft.setCursor(150, FOOT_Y);
    tft.print("CYC");
    tft.setCursor(252, FOOT_Y);
    tft.print("CLK");

    sweep_col = 0;
    have_prev = false;
    shown = { 0xFFFF, 0xFF, 0xFF };
    shown_fetch = 0xFFFFFFFF;
    shown_rate = 1;
    shown_cycles = 1;
    have_fetch = false;
}

static void print_hex(int16_t x, int16_t y, uint16_t value, uint8_t digits, uint16_t color)
{
    char buf[6];
    buf[0] = '$';
    for (int i = digits; i > 0; --i) {
        uint8_t nibble = (value >> ((i - 1) * 4)) & 0xF;
        buf[digits - i + 1] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    buf[digits + 1] = '\0';
    tft.fillRect(x, y, (digits + 1) * 18, 24, COL_BG);
    tft.setTextSize(3);
    tft.setTextColor(color);
    tft.setCursor(x, y);
    tft.print(buf);
}

static void draw_header(const display_sample_t& s)
{
    if (s.addr != shown.addr) {
        print_hex(8, HDR_Y + 12, s.addr, 4, COL_VALUE);
    }
    if (s.data != shown.data) {
        print_hex(140, HDR_Y + 12, s.data, 2, COL_VALUE);
    }
    if ((s.flags & DISP_RW) != (shown.flags & DISP_RW)) {
        bool rd = s.flags & DISP_RW;
        tft.fillRect(252, HDR_Y + 12, 18, 24, COL_BG);
        tft.setTextSize(3);
        tft.setTextColor(rd ? COL_READ : COL_WRITE);
        tft.setCursor(252, HDR_Y + 12);
        tft.print(rd ? 'R' : 'W');
    }
    if (s.flags != shown.flags) {
        // asserted-signal indicators (active-low signals lit when low)
        tft.setTextSize(1);
        for (int i = 1; i < LANE_COUNT; ++i) { // skip PHI2
            const lane_t& lane = LANES[i];
            bool level = s.flags & lane.mask;
            bool lit = (lane.mask & ACTIVE_LOW) ? !level : level;
            tft.setTextColor(lit ? lane.color : COL_LABEL);
            tft.setCursor(8 + (i - 1) * 44, FLAGS_Y);
            tft.print(lane.name);
        }
    }
    shown = s;
}

static void draw_column(const display_sample_t& s)
{
    const int16_t x = WAVE_X + sweep_col * COL_W;
    tft.startWrite(); // batch the whole column into one SPI transaction
    tft.writeFillRect(x, WAVE_Y, COL_W, LANE_COUNT * LANE_H, COL_BG);

    for (int i = 0; i < LANE_COUNT; ++i) {
        const int16_t top = WAVE_Y + i * LANE_H + 2;
        const int16_t bottom = WAVE_Y + i * LANE_H + LANE_H - 3;
        const bool high = s.flags & LANES[i].mask;
        const bool was_high = prev_levels & LANES[i].mask;
        const int16_t y = high ? top : bottom;
        tft.writeFastHLine(x, y, COL_W, LANES[i].color);
        if (have_prev && high != was_high) {
            tft.writeFastVLine(x, top, bottom - top + 1, LANES[i].color);
        }
    }

    prev_levels = s.flags;
    have_prev = true;
    sweep_col = (sweep_col + 1) % WAVE_COLS;
    tft.endWrite();
}

// Sweep cursor ahead of the freshest column - drawn once per refresh
static void draw_cursor()
{
    tft.startWrite();
    tft.writeFillRect(WAVE_X + sweep_col * COL_W, WAVE_Y, COL_W, LANE_COUNT * LANE_H, COL_CURSOR);
    tft.endWrite();
}

static void draw_footer(bool full)
{
    const uint32_t fetch = ((uint32_t) fetch_addr << 8) | fetch_data;
    if (full && have_fetch && fetch != shown_fetch) {
        shown_fetch = fetch;
        tft.fillRect(40, FOOT_Y, 100, 8, COL_BG);
        tft.setTextSize(1);
        tft.setTextColor(COL_SYNC);
        tft.setCursor(40, FOOT_Y);
        char buf[12];
        snprintf(buf, sizeof(buf), "$%04X %02X", fetch_addr, fetch_data);
        tft.print(buf);
    }
    if (full && cycle_count != shown_cycles) {
        shown_cycles = cycle_count;
        tft.fillRect(174, FOOT_Y, 66, 8, COL_BG);
        tft.setTextSize(1);
        tft.setTextColor(COL_VALUE);
        tft.setCursor(174, FOOT_Y);
        tft.print(cycle_count);
    }
    if (rate != shown_rate) {
        shown_rate = rate;
        tft.fillRect(276, FOOT_Y, 44, 8, COL_BG);
        tft.setTextSize(1);
        tft.setTextColor(rate ? COL_PHI2 : COL_LABEL);
        tft.setCursor(276, FOOT_Y);
        if (rate == 0) {
            tft.print("HOLD");
        } else {
            char buf[16];
            snprintf(buf, sizeof(buf), "%luHz", (unsigned long) rate);
            tft.print(buf);
        }
    }
}

//--------------------------------------------------------------------------
// Public API

void display_setup()
{
    SPI1.begin();
    tft.begin(30000000); // ILI9341 handles 30 MHz fine on short PCB traces
    tft.setRotation(DISPLAY_ROTATION);
    tft.fillScreen(COL_BG);
}

void display_capture(const uint8_t payload[5])
{
    const display_sample_t s = decode_sample(payload);
    ring[ring_head++] = s;
    if (ring_head == ring_tail) {
        ++ring_tail; // overwrite the oldest sample
    }

    const bool phi2 = s.flags & DISP_PHI2;
    if (phi2 && !last_phi2) {
        ++cycle_count;
    }
    last_phi2 = phi2;
    if (!(s.flags & DISP_RES)) {
        cycle_count = 0;
    }
    if (phi2 && (s.flags & DISP_SYNC)) { // opcode fetch
        fetch_addr = s.addr;
        fetch_data = s.data;
        have_fetch = true;
    }
    ++captures;
}

void display_refresh()
{
    const uint32_t now = millis();
    if (now - last_refresh_ms < DISPLAY_REFRESH_MS) {
        return;
    }
    last_refresh_ms = now;

    if (now - last_rate_ms >= 1000) {
        rate = captures / 2; // half-cycles -> cycles per second
        captures = 0;
        last_rate_ms = now;
    }

    if (ring_head == ring_tail && chrome_drawn) {
        draw_footer(true); // rate may have dropped to HOLD
        return;
    }

    if (!chrome_drawn) {
        draw_chrome();
        chrome_drawn = true;
        splash_drawn = false;
    }

    // Fast-forward when more samples arrived than we can draw. A backlog
    // means the CPU is free-running - the header values are a blur anyway,
    // so redraw the expensive text at most once per second and spend the
    // budget on the waveform instead. While stepping (no backlog) every
    // detail updates immediately.
    uint8_t backlog = ring_head - ring_tail;
    const bool fast_mode = backlog > MAX_COLS_PER_REFRESH;
    if (fast_mode) {
        ring_tail = ring_head - MAX_COLS_PER_REFRESH;
        have_prev = false;
    }

    display_sample_t latest = { 0, 0, 0 };
    bool drew = false;
    while (ring_tail != ring_head) {
        latest = ring[ring_tail++];
        draw_column(latest);
        drew = true;
    }
    if (drew) {
        draw_cursor();
    }

    const bool full_detail = drew && (!fast_mode || now - last_header_ms >= 1000);
    if (full_detail) {
        draw_header(latest);
        last_header_ms = now;
    }
    draw_footer(full_detail);
}

#endif // ENABLE_DISPLAY && !RUNNING_TESTS
