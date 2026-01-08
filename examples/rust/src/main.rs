mod configuration;
mod pins;
mod protocol;
mod runner;

use configuration::{PORT_NAME, PROGRAM_FILE};
use pins::Pins;
use runner::{load_program, write_byte, Runner};
use serialport::SerialPort;
use std::time::Duration;

fn main() {
    // create 64kB RAM and intialize it with reset vector and load a program
    let mut mem: [u8; 1 << 16] = [0; 1 << 16];
    const PROGRAM_ADDR: u16 = 0x0200;
    initialize_vectors(&mut mem, PROGRAM_ADDR);
    load_program(&mut mem, PROGRAM_FILE, PROGRAM_ADDR);

    // open serial port
    let mut port = serialport::new(PORT_NAME, 115_200)
        .timeout(Duration::from_millis(10000))
        .open()
        .expect("Failed to open port");

    let _ = port.write_data_terminal_ready(true);
    let _ = port.write_request_to_send(true);

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

    runner.run();
}

fn initialize_vectors(mem: &mut [u8; 1 << 16], addr: u16) {
    set_vector(mem, 0xfffc, addr); // RESET
    set_vector(mem, 0xfffa, addr); // NMI
    set_vector(mem, 0xfffe, addr); // IRQ/BRK
}

fn set_vector(mem: &mut [u8; 1 << 16], vector_addr: u16, target: u16) {
    write_byte(mem, vector_addr, (target & 0xff) as u8);
    write_byte(mem, vector_addr + 1, (target >> 8) as u8);
}
