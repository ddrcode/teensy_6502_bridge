use std::fmt;

pub struct Message<const SIZE: usize> {
    pub msg_code: u8,
    pub data: [u8; SIZE],
}

impl<const SIZE: usize> Message<SIZE> {
    pub fn new(code: u8, data: [u8; SIZE]) -> Self {
        Self {
            msg_code: code,
            data
        }
    }

    pub fn from_bytes(bytes: &[u8]) -> Self {
        let msg = Self::new(bytes[0], bytes[1..SIZE+1].try_into().expect("Invalid array size"));
        if msg.checksum() != bytes[SIZE+1] {
            panic!("Checksum error");
        }
        msg
    }

    pub fn size() -> usize {
        SIZE + 2
    }

    pub fn checksum(&self) -> u8 {
        self.data.iter().fold(self.msg_code, |acc, val| acc.wrapping_add(*val))
    }

    pub fn to_vec(&self) -> Vec<u8> {
        let mut msg = Vec::with_capacity(SIZE + 2);
        msg.push(self.msg_code);
        msg.extend_from_slice(&self.data[..]);
        msg.push(self.checksum());
        msg
    }
}

impl<const SIZE: usize> fmt::Display for Message<SIZE> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> Result<(), std::fmt::Error> {
        write!(f, "Code: {:02x}", self.msg_code)?;
        write!(f, ", Data: ")?;
        for i in 0..SIZE {
            write!(f, " {:02x}", self.data[i])?;
        }
        write!(f, ", Checksum: {:02}\n", self.checksum())?;
        Ok(())
    }
}

pub type PinsMsg = Message<5>;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_creation_from_bytes() {
        let bytes = [1u8, 0, 1, 2, 3, 4, 11];
        let msg = PinsMsg::from_bytes(&bytes[..]);
        assert_eq!(msg.checksum(), 11);
    }

    #[test]
    fn test_to_vec() {
        let msg: PinsMsg = PinsMsg::new(1, [0,1,2,3,4]);
        assert_eq!(msg.to_vec(), vec![1, 0, 1, 2, 3, 4, 11]);
    }
}
