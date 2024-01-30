// CONFIGURATION
// Edit this section to adjust the config to your project.

// uncomment the line below if you want to test the project
// without any client app, just serial monitor
// This is handy in case of testing the correctness of your wiring.
// #define DEBUG_TEENSY_COM_BRIDGE

#ifdef DEBUG_TEENSY_COM_BRIDGE
// the clock (in ms) is required only for debug mode.
// The bridge itself is being "clocked" by the COM port
// messages and as such doesn't provide any notion of cycles.
#define CYCLE_DURATION 500
#endif

// PIN CONFIGURATION
// For each W65C02 PIN (as in the comments), provide Teensy 4.1 pin no.
// Pins 8, 21 and 35 are not data pins, so they can be ignored.
// Pins 4, 6, 36 and 40 can be skipped if connected directly to power.
// Pin 2 can be skipped if connected to power via resistor.
#define PINS_MAP \
   2,  /*     VP <-- |  1      40 | <-- RES   */     18, \
  21,  /*    RDY <-> |  2      39 | --> PHI2O */      1, \
   3,  /*  PHI1O <-- |  3      38 | <-- SO    */     17, \
  20,  /*    IRQ --> |  4      37 | <-- PHI2  */     16, \
   4,  /*     ML <-- |  5     @36 | <-- BE    */     15, \
  19,  /*    NMI --> |  6      35 | --- NC    */      0, \
   5,  /*   SYNC <-- |  7     *34 | --> RW    */     14, \
   0,  /*    VDD --> |  8     *33 | <-> D0    */     40, \
   6,  /*     A0 <-- |  9*    *32 | <-> D1    */     39, \
   7,  /*     A1 <-- | 10*    *31 | <-> D2    */     38, \
   8,  /*     A2 <-- | 11*    *30 | <-> D3    */     37, \
   9,  /*     A3 <-- | 12*    *29 | <-> D4    */     36, \
  10,  /*     A4 <-- | 13*    *28 | <-> D5    */     35, \
  11,  /*     A5 <-- | 14*    *27 | <-> D6    */     34, \
  12,  /*     A6 <-- | 15*    *26 | <-> D7    */     33, \
  24,  /*     A7 <-- | 16*    *25 | --> A15   */     29, \
  25,  /*     A8 <-- | 17*    *24 | --> A14   */     30, \
  26,  /*     A9 <-- | 18*    *23 | --> A13   */     31, \
  27,  /*    A10 <-- | 19*    *22 | --> A12   */     32, \
  28,  /*    A11 <-- | 20*     21 | --> GND   */     0

#define PINS_MAP_PCB \
   0,  /*     VP <-- |  1      40 | <-- RES   */     23, \
  21,  /*    RDY <-> |  2      39 | --> PHI2O */     22, \
   2,  /*  PHI1O <-- |  3      38 | <-- SO    */     20, \
   3,  /*    IRQ --> |  4      37 | <-- PHI2  */     13, \
   4,  /*     ML <-- |  5     @36 | <-- BE    */     19, \
   5,  /*    NMI --> |  6      35 | --- NC    */     18, \
   6,  /*   SYNC <-- |  7     *34 | --> RW    */     17, \
   1,  /*    VDD --> |  8     *33 | <-> D0    */     16, \
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
  32,  /*    A11 <-- | 20*     21 | --> GND   */     1
