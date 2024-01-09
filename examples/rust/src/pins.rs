/// A structure representing 65C02 pins.
#[derive(Default, Debug, Copy, Clone)]
pub struct Pins {
    pub vp: bool,
    pub ready: bool,
    pub phi1o: bool,
    pub irq: bool,
    pub ml: bool,
    pub nmi: bool,
    pub sync: bool,
    pub addr: u16,
    pub data: u8,
    pub rw: bool,
    pub be: bool,
    pub phi2: bool,
    pub so: bool,
    pub phi2o: bool,
    pub reset: bool,
}

impl Pins {
    #[inline]
    pub fn is_write(&self) -> bool {
        !self.rw
    }
}

impl std::fmt::Display for Pins {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "Addr: {:04x}, Data: {:02x}, Flags (VP, SYNC, RW): {}{}{}, Inputs (PHI, IRQ, NMI, RST): {}{}{}{}",
            self.addr,
            self.data,
            u8::from(self.vp),
            u8::from(self.sync),
            u8::from(self.rw),
            u8::from(self.phi2),
            u8::from(self.irq),
            u8::from(self.nmi),
            u8::from(self.reset),
        )
    }
}

impl From<u64> for Pins {
    fn from(v: u64) -> Self {
        Pins {
            vp: to_bool(v, 0),
            ready: to_bool(v, 1),
            phi1o: to_bool(v, 2),
            irq: to_bool(v, 3),
            ml: to_bool(v, 4),
            nmi: to_bool(v, 5),
            sync: to_bool(v, 6),
            addr: (((v >> 8) & 0b1111_1111_1111) | ((v >> 9) & 0b1111_0000_0000_0000)) as u16,
            data: (((v >> 25) & 0xff) as u8).reverse_bits(),
            rw: to_bool(v, 33),
            be: to_bool(v, 35),
            phi2: to_bool(v, 36),
            so: to_bool(v, 37),
            phi2o: to_bool(v, 38),
            reset: to_bool(v, 39),
        }
    }
}

impl From<[u8; 5]> for Pins {
    fn from(value: [u8; 5]) -> Self {
        let mut bytes: [u8; 8] = [0; 8];
        for i in 0..5 {
            bytes[i + 3] = value[i];
        }
        Pins::from(u64::from_be_bytes(bytes))
    }
}

impl From<Pins> for u64 {
    fn from(pins: Pins) -> Self {
        let mut v = 0u64;
        from_bool(&mut v, pins.vp, 0);
        from_bool(&mut v, pins.ready, 1);
        from_bool(&mut v, pins.phi1o, 2);
        from_bool(&mut v, pins.irq, 3);
        from_bool(&mut v, pins.ml, 4);
        from_bool(&mut v, pins.nmi, 5);
        from_bool(&mut v, pins.sync, 6);
        from_bool(&mut v, pins.rw, 33);
        from_bool(&mut v, pins.be, 35);
        from_bool(&mut v, pins.phi2, 36);
        from_bool(&mut v, pins.so, 37);
        from_bool(&mut v, pins.phi2o, 38);
        from_bool(&mut v, pins.reset, 39);
        v |= u64::from(pins.data.reverse_bits()) << 25;
        v |= u64::from(pins.addr & 0b0000_1111_1111_1111) << 8;
        v |= u64::from(pins.addr & 0b1111_0000_0000_0000) << 9;
        v
    }
}

impl From<Pins> for [u8; 5] {
    fn from(pins: Pins) -> Self {
        let val: u64 = pins.into();
        val.to_be_bytes()
            .split_at(3)
            .1
            .try_into()
            .expect("The Pins slice must be 5-elements long")
    }
}

#[inline]
fn to_bool(val: u64, shift: u8) -> bool {
    (val & (1 << (shift as u64))) != 0
}

fn from_bool(num: &mut u64, val: bool, bit: u8) {
    if val {
        *num |= 1 << bit;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_conversions() {
        //                    RES  BE   D1   D5   A14    A8   A4   A0  MLB   VP
        let pins_num: u64 = 0b1101_1000_1100_0000_1000_0000_0000_0100_0000_0001;
        let pins: Pins = pins_num.into();

        assert!(pins.reset);
        assert!(pins.vp);
        assert!(pins.phi2);
        assert!(pins.phi2o);
        assert!(!pins.phi1o);
        assert_eq!(0b0100_0000_0000_0100, pins.addr);
        assert_eq!(0b0000_0110, pins.data);

        assert_eq!(pins_num, u64::from(pins));
    }

    #[test]
    fn test_reset_vector() {
        let mut pins = Pins::default();
        pins.addr = 0xfffd;
        pins.data = 0x02;

        let num: u64 = pins.into();
        let out = Pins::from(num);

        assert_eq!(0xfffd, out.addr);
        assert_eq!(0x02, out.data);
    }
}

