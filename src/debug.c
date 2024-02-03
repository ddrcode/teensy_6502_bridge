#include "debug.h"

void loop_debug()
{
    static int cycle = 0;
    static bool phase = true;

    digitalToggle(pins.phi2);
    delay(CYCLE_DURATION);
    handle_cycle(pins);
    if (phase) print_status(pins);
    ++cycle;
    phase = !phase;
}

void print_pin(char* label, uint8_t pin)
{
    Serial.print(", ");
    Serial.print(label);
    Serial.print("=");
    Serial.print(read_pin(pin) == HIGH ? 1 : 0);
}

void print_status(pins_t &pins)
{
    Serial.print("Addr=");
    Serial.print(get_val_from_pins(pins.addr, 16), HEX);
    Serial.print(", Data=");
    Serial.print(get_val_from_pins(pins.data, 8), BIN);
    print_pin("RW", pins.rw);
    print_pin("SYNC", pins.sync);
    print_pin("VP", pins.vp);
    print_pin("ML", pins.ml);
    Serial.println();
}
