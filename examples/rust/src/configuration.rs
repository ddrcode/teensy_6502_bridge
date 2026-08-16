use std::time::Duration;

// Extra delay per half-cycle. The bridge replies only after the half-cycle
// is complete, so no delay is needed for correctness - increase this value
// to deliberately slow down execution (i.e. to observe board LEDs).
pub const CYCLE_DURATION: Duration = Duration::ZERO;
pub const SHOW_RAW_DATA: bool = false;
