//! Serial message protocol (see `docs/protocol.md`).
//!
//! Every message is `type + payload + checksum`, where the checksum is the
//! 8-bit wrapping sum of the type byte and all payload bytes.

use crate::payload::BUFF_SIZE;

pub const MSG_ERROR: u8 = 0;
pub const MSG_STATUS: u8 = 1;
pub const MSG_PINS: u8 = 2;

pub const ERR_INVALID_MSG_TYPE: u8 = 1;
pub const ERR_INVALID_CHECKSUM: u8 = 2;

/// Total wire size of a pins message.
pub const PINS_MSG_SIZE: usize = BUFF_SIZE + 2;

/// Total wire size of a message of the given type, or `None` when the type
/// byte is not a known message type.
pub const fn msg_size(msg_type: u8) -> Option<usize> {
    match msg_type {
        MSG_ERROR | MSG_STATUS => Some(3),
        MSG_PINS => Some(PINS_MSG_SIZE),
        _ => None,
    }
}

/// 8-bit wrapping sum of all bytes.
pub fn checksum(bytes: &[u8]) -> u8 {
    bytes.iter().fold(0u8, |acc, b| acc.wrapping_add(*b))
}

/// Builds the 3-byte error message for the given code.
pub fn error_msg(code: u8) -> [u8; 3] {
    [MSG_ERROR, code, MSG_ERROR.wrapping_add(code)]
}

/// The result of feeding one byte into the [`Accumulator`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Event {
    /// The message is not complete yet.
    Pending,
    /// The byte is not a known message type; it has been consumed.
    UnknownType,
    /// A complete message has been received.
    Complete {
        msg_type: u8,
        payload: [u8; BUFF_SIZE],
        checksum_ok: bool,
    },
}

/// Assembles messages from a serial byte stream, mirroring the C++
/// firmware's `try_read_msg` semantics: an unknown type byte is consumed
/// alone, a known type collects its full frame.
#[derive(Default)]
pub struct Accumulator {
    buf: [u8; PINS_MSG_SIZE],
    len: usize,
    total: usize,
}

impl Accumulator {
    pub const fn new() -> Self {
        Self {
            buf: [0; PINS_MSG_SIZE],
            len: 0,
            total: 0,
        }
    }

    /// Drops any partially accumulated message (e.g. when the host
    /// disconnects mid-frame).
    pub fn clear(&mut self) {
        self.len = 0;
        self.total = 0;
    }

    pub fn push(&mut self, byte: u8) -> Event {
        if self.len == 0 {
            match msg_size(byte) {
                None => return Event::UnknownType,
                Some(total) => self.total = total,
            }
        }
        self.buf[self.len] = byte;
        self.len += 1;
        if self.len < self.total {
            return Event::Pending;
        }

        let total = self.total;
        let msg_type = self.buf[0];
        let mut payload = [0u8; BUFF_SIZE];
        payload[..total - 2].copy_from_slice(&self.buf[1..total - 1]);
        let checksum_ok = checksum(&self.buf[..total - 1]) == self.buf[total - 1];
        self.clear();
        Event::Complete {
            msg_type,
            payload,
            checksum_ok,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sizes() {
        assert_eq!(msg_size(MSG_ERROR), Some(3));
        assert_eq!(msg_size(MSG_STATUS), Some(3));
        assert_eq!(msg_size(MSG_PINS), Some(7));
        assert_eq!(msg_size(9), None);
        assert_eq!(msg_size(255), None);
    }

    #[test]
    fn error_message_layout() {
        assert_eq!(error_msg(ERR_INVALID_CHECKSUM), [0, 2, 2]);
        assert_eq!(error_msg(ERR_INVALID_MSG_TYPE), [0, 1, 1]);
    }

    #[test]
    fn accumulates_a_pins_message() {
        let mut acc = Accumulator::new();
        let frame = [2u8, 1, 2, 3, 4, 5, 17];
        for &b in &frame[..6] {
            assert_eq!(acc.push(b), Event::Pending);
        }
        match acc.push(frame[6]) {
            Event::Complete {
                msg_type,
                payload,
                checksum_ok,
            } => {
                assert_eq!(msg_type, MSG_PINS);
                assert_eq!(payload, [1, 2, 3, 4, 5]);
                assert!(checksum_ok);
            }
            other => panic!("unexpected event: {other:?}"),
        }
    }

    #[test]
    fn detects_bad_checksum() {
        let mut acc = Accumulator::new();
        let frame = [2u8, 1, 2, 3, 4, 5, 18];
        let mut last = Event::Pending;
        for &b in &frame {
            last = acc.push(b);
        }
        assert!(matches!(last, Event::Complete { checksum_ok: false, .. }));
    }

    #[test]
    fn consumes_unknown_type_alone() {
        let mut acc = Accumulator::new();
        assert_eq!(acc.push(9), Event::UnknownType);
        // the accumulator is still empty and in sync
        assert_eq!(acc.push(2), Event::Pending);
    }

    #[test]
    fn clear_drops_partial_frame() {
        let mut acc = Accumulator::new();
        acc.push(2);
        acc.push(1);
        acc.clear();
        assert_eq!(acc.push(9), Event::UnknownType);
    }
}
