mod configuration;
mod pins;
mod runner;
mod protocol;

use configuration::{PORT_NAME, PROGRAM_FILE};
use pins::Pins;
use runner::{load_program, write_byte, Runner};
use serialport;
use std::time::Duration;

fn main() {
    // create 64kB RAM and intialize it with reset vector and load a program
    let mut mem: [u8; 1 << 16] = [0; 1 << 16];
    write_byte(&mut mem, 0xfffd, 0x02);
    load_program(&mut mem, PROGRAM_FILE, 0x0200);

    // open serial port
    let port = serialport::new(PORT_NAME, 115_200)
        .timeout(Duration::from_millis(10000))
        .open()
        .expect("Failed to open port");

    // initialize logic (program runner)
    let mut runner = Runner {
        cycle: 0,
        mem,
        port,
        phase: false,
        addr: 0,
        write: false,
        pins: Pins::default(),
    };

    // reset the CPU
    runner.reset();

    // step-through until BRK instruction
    while runner.step() {}
}
