#include <cstddef>
#include <cstring>

#include "diagnostics.hpp"
#include "hardware.hpp"
#include "cpu.hpp"
#include "pins.hpp"

namespace {
constexpr uint32_t HALF_CYCLE_DELAY_US = 1;
constexpr uint8_t CYCLES_PER_LOOP = 4;

constexpr uint16_t ROM_BASE = 0xE000;
constexpr uint16_t ROM_SIZE = 0x2000;
constexpr uint16_t RAM_BASE = 0x6000;
constexpr uint16_t RAM_SIZE = 0x2000;
constexpr uint16_t PATTERNS_ADDR = 0xE02E;
constexpr uint16_t DATA_ADDR = 0x6000;
constexpr uint16_t ADDRESS_TEST_BASE = 0x7000;
constexpr uint16_t REPORT_ADDR = 0x7FF0;
constexpr uint8_t REPORT_VALUE = 0xAA;
constexpr uint8_t HIGH_ADDR_VALUE = 0x5A;
constexpr uint16_t HIGH_ADDRS[] = { 0x8000, 0xA000, 0xC000, 0xE000 };

constexpr uint8_t DATA_PATTERNS[] = {
    0x00, 0xFF, 0x55, 0xAA,
    0x33, 0xCC, 0x0F, 0xF0,
    0x11, 0x22, 0x44, 0x88,
    0x77, 0x7F, 0xFE, 0xEF
};
constexpr size_t DATA_PATTERN_COUNT = sizeof(DATA_PATTERNS) / sizeof(DATA_PATTERNS[0]);
constexpr size_t HIGH_ADDR_COUNT = sizeof(HIGH_ADDRS) / sizeof(HIGH_ADDRS[0]);

struct RomByte {
    uint16_t addr;
    uint8_t value;
};

constexpr RomByte ROM_CODE[] = {
    { 0xE000, 0x78 }, // SEI
    { 0xE001, 0xD8 }, // CLD
    { 0xE002, 0xA2 }, { 0xE003, 0x00 }, // LDX #$00
    { 0xE004, 0xBD }, { 0xE005, 0x2E }, { 0xE006, 0xE0 }, // LDA patterns,X
    { 0xE007, 0x8D }, { 0xE008, 0x00 }, { 0xE009, 0x60 }, // STA $6000
    { 0xE00A, 0xE8 },                         // INX
    { 0xE00B, 0xE0 }, { 0xE00C, 0x10 },       // CPX #$10
    { 0xE00D, 0xD0 }, { 0xE00E, 0xF5 },       // BNE E004
    { 0xE00F, 0xA2 }, { 0xE010, 0x00 },       // LDX #$00
    { 0xE011, 0x8A },                         // TXA
    { 0xE012, 0x9D }, { 0xE013, 0x00 }, { 0xE014, 0x70 }, // STA $7000,X
    { 0xE015, 0xE8 },                         // INX
    { 0xE016, 0xD0 }, { 0xE017, 0xF9 },       // BNE E011
    { 0xE018, 0xA9 }, { 0xE019, 0x5A },       // LDA #$5A
    { 0xE01A, 0x8D }, { 0xE01B, 0x00 }, { 0xE01C, 0x80 }, // STA $8000
    { 0xE01D, 0x8D }, { 0xE01E, 0x00 }, { 0xE01F, 0xA0 }, // STA $A000
    { 0xE020, 0x8D }, { 0xE021, 0x00 }, { 0xE022, 0xC0 }, // STA $C000
    { 0xE023, 0x8D }, { 0xE024, 0x00 }, { 0xE025, 0xE0 }, // STA $E000
    { 0xE026, 0xA9 }, { 0xE027, 0xAA },       // LDA #$AA
    { 0xE028, 0x8D }, { 0xE029, 0xF0 }, { 0xE02A, 0x7F }, // STA $7FF0
    { 0xE02B, 0x4C }, { 0xE02C, 0x2B }, { 0xE02D, 0xE0 }  // JMP $E02B
};

enum class Phase : uint8_t {
    DataBus = 0,
    AddressBus,
    HighAddress,
    Completion
};

struct Issue {
    const char* stage = nullptr;
    uint16_t expected_addr = 0;
    uint8_t expected_value = 0;
    uint16_t observed_addr = 0;
    uint8_t observed_value = 0;
};

struct Report {
    bool data_passed = false;
    bool address_passed = false;
    bool high_passed = false;
    bool done = false;
    bool error = false;
    bool printed = false;
    Issue issue{};
};

uint8_t rom[ROM_SIZE];
uint8_t ram[RAM_SIZE];
bool initialized = false;
bool halted = false;
Phase phase = Phase::DataBus;
size_t pattern_index = 0;
uint16_t address_index = 0;
uint8_t high_index = 0;
Report report;

void init_memory();
void run_cycle(pins_t& pins);
uint8_t read_memory(uint16_t addr);
void write_memory(uint16_t addr, uint8_t value);
void monitor_write(uint16_t addr, uint8_t value);
void set_error(const char* stage, uint16_t expected_addr, uint8_t expected_value,
               uint16_t observed_addr, uint8_t observed_value);
void maybe_report();
void print_hex16(uint16_t value);
void print_hex8(uint8_t value);

} // namespace

void loop_diagnostics(pins_t& pins)
{
    if (!initialized) {
        init_memory();
        Serial.println(F("Diagnostics: starting 6502 connection tests"));
    }

    if (halted) {
        digitalWriteFast(pins.phi2, LOW);
        maybe_report();
        return;
    }

    for (uint8_t i = 0; i < CYCLES_PER_LOOP; ++i) {
        run_cycle(pins);
        if (halted) break;
    }

    if (halted) {
        digitalWriteFast(pins.phi2, LOW);
    }

    maybe_report();
}

namespace {

void init_memory()
{
    std::memset(rom, 0, sizeof(rom));
    std::memset(ram, 0, sizeof(ram));

    for (const auto& entry : ROM_CODE) {
        const uint16_t idx = entry.addr - ROM_BASE;
        if (idx < ROM_SIZE) {
            rom[idx] = entry.value;
        }
    }

    for (size_t i = 0; i < DATA_PATTERN_COUNT; ++i) {
        const uint16_t addr = PATTERNS_ADDR + static_cast<uint16_t>(i);
        const uint16_t idx = addr - ROM_BASE;
        if (idx < ROM_SIZE) {
            rom[idx] = DATA_PATTERNS[i];
        }
    }

    // Reset/IRQ vectors
    rom[0xFFFC - ROM_BASE] = 0x00;
    rom[0xFFFD - ROM_BASE] = 0xE0;
    rom[0xFFFE - ROM_BASE] = 0x00;
    rom[0xFFFF - ROM_BASE] = 0xE0;

    report = Report{};
    pattern_index = 0;
    address_index = 0;
    high_index = 0;
    phase = Phase::DataBus;
    halted = false;
    initialized = true;
}

uint8_t read_memory(const uint16_t addr)
{
    if (addr >= ROM_BASE && addr < ROM_BASE + ROM_SIZE) {
        return rom[addr - ROM_BASE];
    }
    if (addr >= RAM_BASE && addr < RAM_BASE + RAM_SIZE) {
        return ram[addr - RAM_BASE];
    }
    return 0x00;
}

void write_memory(const uint16_t addr, const uint8_t value)
{
    if (addr >= RAM_BASE && addr < RAM_BASE + RAM_SIZE) {
        ram[addr - RAM_BASE] = value;
    }
    monitor_write(addr, value);
}

void run_cycle(pins_t& pins)
{
    if (report.error || report.done) {
        halted = true;
        return;
    }

    digitalWriteFast(pins.phi2, LOW);
    delayMicroseconds(HALF_CYCLE_DELAY_US);
    handle_cycle(pins);

    const bool is_read = read_pin(pins.rw) == HIGH;
    const uint16_t addr = get_val_from_pins(pins.addr, 16);

    if (is_read) {
        const uint8_t data = read_memory(addr);
        write_data_bus(pins, data);
    }

    digitalWriteFast(pins.phi2, HIGH);
    delayMicroseconds(HALF_CYCLE_DELAY_US);

    if (!is_read) {
        const uint8_t data = read_data_bus(pins);
        write_memory(addr, data);
    }
}

void set_error(const char* stage, const uint16_t expected_addr, const uint8_t expected_value,
               const uint16_t observed_addr, const uint8_t observed_value)
{
    if (report.error) return;
    report.error = true;
    report.issue.stage = stage;
    report.issue.expected_addr = expected_addr;
    report.issue.expected_value = expected_value;
    report.issue.observed_addr = observed_addr;
    report.issue.observed_value = observed_value;
    halted = true;
}

void monitor_write(const uint16_t addr, const uint8_t value)
{
    if (report.error || report.done) {
        halted = true;
        return;
    }

    switch (phase) {
        case Phase::DataBus:
            if (addr != DATA_ADDR || value != DATA_PATTERNS[pattern_index]) {
                set_error("data-bus", DATA_ADDR, DATA_PATTERNS[pattern_index], addr, value);
                return;
            }
            ++pattern_index;
            if (pattern_index >= DATA_PATTERN_COUNT) {
                report.data_passed = true;
                phase = Phase::AddressBus;
            }
            break;

        case Phase::AddressBus: {
            const uint16_t expected_addr = ADDRESS_TEST_BASE + address_index;
            const uint8_t expected_value = static_cast<uint8_t>(address_index & 0xFF);
            if (addr != expected_addr || value != expected_value) {
                set_error("address-bus", expected_addr, expected_value, addr, value);
                return;
            }
            ++address_index;
            if (address_index >= 0x100) {
                report.address_passed = true;
                phase = Phase::HighAddress;
            }
            break;
        }

        case Phase::HighAddress: {
            if (high_index >= HIGH_ADDR_COUNT) {
                phase = Phase::Completion;
                break;
            }
            const uint16_t expected_addr = HIGH_ADDRS[high_index];
            if (addr != expected_addr || value != HIGH_ADDR_VALUE) {
                set_error("high-address", expected_addr, HIGH_ADDR_VALUE, addr, value);
                return;
            }
            ++high_index;
            if (high_index >= HIGH_ADDR_COUNT) {
                report.high_passed = true;
                phase = Phase::Completion;
            }
            break;
        }

        case Phase::Completion:
            if (addr == REPORT_ADDR && value == REPORT_VALUE) {
                report.done = true;
                halted = true;
            } else {
                set_error("completion", REPORT_ADDR, REPORT_VALUE, addr, value);
            }
            break;
    }
}

char to_hex(const uint8_t nibble)
{
    return nibble < 10 ? static_cast<char>('0' + nibble) : static_cast<char>('A' + nibble - 10);
}

void print_hex16(const uint16_t value)
{
    for (int shift = 12; shift >= 0; shift -= 4) {
        Serial.print(to_hex((value >> shift) & 0xF));
    }
}

void print_hex8(const uint8_t value)
{
    for (int shift = 4; shift >= 0; shift -= 4) {
        Serial.print(to_hex((value >> shift) & 0xF));
    }
}

void maybe_report()
{
    if (report.printed) return;
    if (!Serial.dtr()) return;

    if (report.error) {
        Serial.print(F("Diagnostics FAILED during "));
        Serial.print(report.issue.stage);
        Serial.println(F(" test"));
        Serial.print(F("Expected addr 0x"));
        print_hex16(report.issue.expected_addr);
        Serial.print(F(" = 0x"));
        print_hex8(report.issue.expected_value);
        Serial.print(F(", got addr 0x"));
        print_hex16(report.issue.observed_addr);
        Serial.print(F(" = 0x"));
        print_hex8(report.issue.observed_value);
        Serial.println();
        report.printed = true;
        return;
    }

    if (report.done) {
        Serial.println(F("Diagnostics PASSED"));
        Serial.println(F("Data bus, address bus, and high address lines verified."));
        report.printed = true;
    }
}

} // namespace

