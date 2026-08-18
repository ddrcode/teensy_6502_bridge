// CONFIGURATION
// Edit this file to adjust the config of Teensy Bridge.

// Uncomment the line below if you want to test the project
// without any client app, serial monitor (like Arduino
// Serial Monitor or screen).
// This is handy in case of testing the correctness of your wiring.

// #define DEBUG_TEENSY_BRIDGE

// Uncomment to build bridge with on-board diagnostics firmware instead of
// serial-controlled bridge loop.
// #define ENABLE_DIAGNOSTICS

// the clock (in ms) is required only for debug mode.
// The bridge itself is being "clocked" by the COM port
// messages and as such doesn't provide any notion of cycles.
#define CYCLE_DURATION 500

// Uncomment to enable the optional ILI9341 status display connected to the
// PCB's LCD header (SPI1: MOSI=26, SCK=27, MISO=1, D/C=10; CS grounded and
// RST pulled high on the board). Shows a live logic analyzer of the CPU
// signals, address/data readouts and the effective clock rate.
// Requires libraries: Adafruit ILI9341, Adafruit GFX Library, Adafruit BusIO
//   arduino-cli lib install "Adafruit ILI9341" "Adafruit GFX Library" "Adafruit BusIO"
// Note: refreshing the screen costs some throughput in free-run mode
// (roughly 15%); it has no cost while the CPU is stepped or idle.
// #define ENABLE_DISPLAY

#define DISPLAY_PIN_DC 10
#define DISPLAY_PIN_CS -1   // tied to GND on the PCB
#define DISPLAY_PIN_RST -1  // pulled high on the PCB
#define DISPLAY_ROTATION 1  // 1 or 3, depending on the screen orientation
#define DISPLAY_REFRESH_MS 100

// PIN CONFIGURATION
// For each W65C02 PIN (as in the comments), provide Teensy 4.1 pin no.
// Pins 8, 21 and 35 are not data pins, so they can be ignored.
// Pins 2, 4, 6, 36 can be skipped if connected directly to power
// (resistors should be used).
// Value 255 means "ignore".
// In case of W65C816 set pin 35 to 18
#define PINS_MAP\
   0,  /*     VP <-- |  1      40 | <-- RES   */     23, \
  21,  /*    RDY <-> |  2      39 | --> PHI2O */     22, \
   2,  /*  PHI1O <-- |  3      38 | <-- SO    */     20, \
   3,  /*    IRQ --> |  4      37 | <-- PHI2  */     13, \
   4,  /*     ML <-- |  5     @36 | <-- BE    */     19, \
   5,  /*    NMI --> |  6      35 | --- NC    */    255, \
   6,  /*   SYNC <-- |  7     *34 | --> RW    */     17, \
 255,  /*    VDD --> |  8     *33 | <-> D0    */     16, \
   7,  /*     A0 <-- |  9*    *32 | <-> D1    */     15, \
   8,  /*     A1 <-- | 10*    *31 | <-> D2    */     14, \
   9,  /*     A2 <-- | 11*    *30 | <-> D3    */     41, \
  11,  /*     A3 <-- | 12*    *29 | <-> D4    */     40, \
  12,  /*     A4 <-- | 13*    *28 | <-> D5    */     39, \
  24,  /*     A5 <-- | 14*    *27 | <-> D6    */     38, \
  25,  /*     A6 <-- | 15*    *26 | <-> D7    */     37, \
  28,  /*     A7 <-- | 16*    *25 | --> A15   */     36, \
  29,  /*     A8 <-- | 17*    *24 | --> A14   */     35, \
  30,  /*     A9 <-- | 18*    *23 | --> A13   */     34, \
  31,  /*    A10 <-- | 19*    *22 | --> A12   */     33, \
  32,  /*    A11 <-- | 20*     21 | --> GND   */    255

constexpr bool SERIAL_ENABLED = true;
constexpr bool ETHERNET_ENABLED = false;

#define THROW_ERROR_IF_NOT_FAST

