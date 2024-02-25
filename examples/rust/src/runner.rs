use crate::{
    configuration::{CYCLE_DURATION, SHOW_RAW_DATA},
    pins::Pins,
    protocol::PinsMsg
};
use serialport::SerialPort;
use std::{fs::File, io::{self, Read}, path::PathBuf, thread::sleep};

pub struct Runner {
    pub cycle: u64,
    pub mem: [u8; 1 << 16],
    pub port: Box<dyn SerialPort>,
    pub phase: bool,
    pub addr: u16,
    pub write: bool,
    pub pins: Pins,
}

impl Runner {
    /// It executes a single half-step (high or low clock signal) of the CPU.
    /// For every exection it sends data to CPU, adjusting the clock status (PHI2) first,
    /// and then it reads status back from the CPU.
    /// For every low half-cycle it reads the address and the RWB status (pin 34).
    /// For every high half-cycle it does the actual memory operation.
    /// At the end the method advances the clock (switches the phase and increments the
    /// clock counter).
    /// The method returns boolean value indicating whether the program reached an end,
    /// that is assumed to be represented by BRK instruction (opcode 0).
    pub fn step(&mut self) -> bool {
        self.pins.phi2 = self.phase;
        self.write_port();

        sleep(CYCLE_DURATION);
        self.read_port();

        if !self.phase {
            self.addr = self.pins.addr;
            self.write = self.pins.is_write();
            if !self.write {
                self.pins.data = read_byte(&self.mem, self.addr);
            }
        } else {
            if self.write {
                write_byte(&mut self.mem, self.addr, self.pins.data);
            }
            println!("[{}] {}", self.cycle, self.pins);
            if self.pins.sync && self.pins.data == 0 && self.cycle > 20 {
                return false;
            }
        }

        self.advance_cycles();
        true
    }

    /// Resets the CPU, by holding RESB signal (pin 40) low for two cycles (4 half-cycles).
    pub fn reset(&mut self) {
        let mut pins = Pins::default();
        pins.be = true;
        pins.rw = true;
        pins.irq = true;
        pins.nmi = true;
        pins.so = false;

        for _ in 0..4 {
            self.pins = pins.clone(); // Pins::from(pins);
            self.pins.phi2 = self.phase;
            self.write_port();

            sleep(CYCLE_DURATION);
            self.read_port();
            self.advance_cycles();
        }

        self.pins.data = 0;
        self.pins.addr = 0;
        self.pins.reset = true;
        self.pins.ready = true;
        self.pins.so = true;
    }

    /// Reads 5-bytes buffer from the serial port and updates
    /// `pins` field.
    fn read_port(&mut self) {
        let mut buff: [u8; 7] = [0; 7];
        self.port
            .read(&mut buff)
            .expect("Read error from serial port");
        let msg = PinsMsg::from_bytes(&buff[..]);
        if SHOW_RAW_DATA {
            println!("Reading: {}", msg);
            // print_buff(&buff);
        }
        self.pins = Pins::from(msg.data);
    }

    /// Send the value of `pins` field into to serial port.
    fn write_port(&mut self) {
        // let buff: [u8; 5] = self.pins.into();
        let msg = PinsMsg::new(2, self.pins.into());
        if SHOW_RAW_DATA {
            print!("Writing: {}", msg);
            // print_buff(&buff);
        }
        // self.port.write(&msg.to_vec()).expect("Write error to serial port");
        self.port.write(&[0x02u8, 0x0a, 0x00, 0x00, 0x00, 0x28, 52]).expect("Write error to serial port");
    }

    /// Changes phase of the clock and advances the clock count.
    fn advance_cycles(&mut self) {
        if self.phase {
            self.cycle += 1;
        }
        self.phase = !self.phase;
    }
}

/// Writes a byte under a given address to memory emulation.
pub fn write_byte(mem: &mut [u8], addr: u16, val: u8) {
    mem[addr as usize] = val;
}

/// Reads a byte from a given address of memeory emulation.
pub fn read_byte(mem: &[u8], addr: u16) -> u8 {
    mem[addr as usize]
}

/// Reads contnet of a 6502 program and returns it as
/// a vector of bytes.
fn get_file_as_byte_vec(filename: &PathBuf) -> io::Result<Vec<u8>> {
    let mut f = File::open(filename)?;
    let mut buffer = Vec::new();
    f.read_to_end(&mut buffer)?;
    Ok(buffer)
}

/// Loads 6502 program into memory emulation under a given address.
pub fn load_program(mem: &mut [u8], file: &str, addr: u16) {
    let prg = get_file_as_byte_vec(&PathBuf::from(file)).expect("Can't open program file");
    for (i, byte) in prg.iter().enumerate() {
        write_byte(mem, addr + (i as u16), *byte);
    }
}

/// Prints serial port buffer in a form of 4x8 bits.
fn print_buff(buff: &[u8]) {
    for i in 0..buff.len() {
        print!("{:08b} ", buff[i]);
    }
    println!();
}
