//! The 40-bit pin payload of a pins message.
//!
//! Wire format (see `docs/protocol.md`): bit N of the 40-bit payload is
//! CPU pin N+1; payload byte 0 holds bits 39..32 (big-endian byte order).

/// Payload size in bytes (40 pins, one bit each).
pub const BUFF_SIZE: usize = 5;

/// Wire bit positions (`CPU pin - 1`) of all W65C02 signals.
pub mod bits {
    pub const VP: u8 = 0;
    pub const RDY: u8 = 1;
    pub const PHI1O: u8 = 2;
    pub const IRQ: u8 = 3;
    pub const ML: u8 = 4;
    pub const NMI: u8 = 5;
    pub const SYNC: u8 = 6;
    // bit 7 is VDD (not transferred)
    pub const A0: u8 = 8; // A0-A11 at bits 8-19
    // bit 20 is VSS (not transferred)
    pub const A12: u8 = 21; // A12-A15 at bits 21-24
    pub const D7: u8 = 25; // D7 down to D0 at bits 25-32
    pub const RW: u8 = 33;
    // bit 34 is NC
    pub const BE: u8 = 35;
    pub const PHI2: u8 = 36;
    pub const SO: u8 = 37;
    pub const PHI2O: u8 = 38;
    pub const RES: u8 = 39;
}

/// Wire bit of address line `An` (n in 0..16).
#[inline]
pub const fn addr_bit(n: u8) -> u8 {
    if n < 12 {
        bits::A0 + n
    } else {
        bits::A12 + (n - 12)
    }
}

/// Wire bit of data line `Dn` (n in 0..8). Data bits are reversed on the wire.
#[inline]
pub const fn data_bit(n: u8) -> u8 {
    bits::D7 + (7 - n)
}

/// Reads a single payload bit.
#[inline]
pub fn get_bit(payload: &[u8; BUFF_SIZE], bit: u8) -> bool {
    payload[4 - (bit >> 3) as usize] & (1 << (bit & 7)) != 0
}

/// Writes a single payload bit.
#[inline]
pub fn set_bit(payload: &mut [u8; BUFF_SIZE], bit: u8, value: bool) {
    let byte = &mut payload[4 - (bit >> 3) as usize];
    let mask = 1 << (bit & 7);
    if value {
        *byte |= mask;
    } else {
        *byte &= !mask;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn encode_addr(payload: &mut [u8; BUFF_SIZE], addr: u16) {
        for i in 0..16 {
            set_bit(payload, addr_bit(i), addr & (1 << i) != 0);
        }
    }

    fn decode_addr(payload: &[u8; BUFF_SIZE]) -> u16 {
        (0..16).fold(0, |acc, i| {
            acc | ((get_bit(payload, addr_bit(i)) as u16) << i)
        })
    }

    fn decode_data(payload: &[u8; BUFF_SIZE]) -> u8 {
        (0..8).fold(0, |acc, i| {
            acc | ((get_bit(payload, data_bit(i)) as u8) << i)
        })
    }

    #[test]
    fn addr_roundtrip() {
        let mut p = [0u8; BUFF_SIZE];
        encode_addr(&mut p, 0xABCD);
        assert_eq!(decode_addr(&p), 0xABCD);
    }

    #[test]
    fn data_reversal() {
        let mut p = [0u8; BUFF_SIZE];
        for i in 0..8 {
            set_bit(&mut p, data_bit(i), 0xB1 & (1 << i) != 0);
        }
        assert_eq!(decode_data(&p), 0xB1);
        // D7 (bit 7 of the value) lives at wire bit 25
        assert!(get_bit(&p, 25));
    }

    #[test]
    fn real_hardware_capture() {
        // Payload of an actual bridge reply captured on hardware (during
        // reset, low phase): addr=$0239, data=$00, RW/IRQ/NMI/ML high,
        // VP/SYNC/PHI2/RES low.
        let p: [u8; BUFF_SIZE] = [0x0A, 0x00, 0x02, 0x39, 0x3E];
        assert_eq!(decode_addr(&p), 0x0239);
        assert_eq!(decode_data(&p), 0x00);
        assert!(get_bit(&p, bits::RW));
        assert!(get_bit(&p, bits::IRQ));
        assert!(get_bit(&p, bits::NMI));
        assert!(get_bit(&p, bits::ML));
        assert!(!get_bit(&p, bits::VP));
        assert!(!get_bit(&p, bits::SYNC));
        assert!(!get_bit(&p, bits::PHI2));
        assert!(!get_bit(&p, bits::RES));
    }
}
