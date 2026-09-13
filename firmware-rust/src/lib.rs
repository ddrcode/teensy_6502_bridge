//! Hardware-independent logic of the bridge firmware.
//!
//! This crate compiles as `no_std` for the Teensy target, and as a normal
//! `std` crate for host-side unit tests (`make test`).

#![cfg_attr(not(test), no_std)]

pub mod payload;
pub mod protocol;
