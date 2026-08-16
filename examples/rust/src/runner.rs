use crate::{
    configuration::{CYCLE_DURATION, SHOW_RAW_DATA},
    pins::Pins,
    protocol::PinsMsg,
};
use serialport::SerialPort;
use std::{
    fs::File,
    io::{self, Read, Write},
    path::PathBuf,
    thread::sleep,
};

pub struct Runner {
    pub cycle: u64,
    pub mem: [u8; 1 << 16],
    pub port: Box<dyn SerialPort>,
    pub phase: bool,
    pub addr: u16,
    pub write: bool,
    pub pins: Pins,
    pub show_halfcycles: bool,
}

impl Runner {
    pub fn run(&mut self) {
        self.reset();
        while self.step() {}
    }
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
            self.print_state();
            if self.pins.sync && self.pins.data == 0 && self.cycle > 20 {
                return false;
            }
        }

        if !self.phase {
            if self.show_halfcycles {
                self.print_state();
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
        pins.reset = false;

        for _ in 0..4 {
            self.pins = pins;
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

    fn print_state(&self) {
        let rw_char = if self.pins.rw { 'R' } else { 'W' };
        let phase_char = if self.phase { 'H' } else { 'L' };
        print!(
            "Cycle={cycle:06} Half={phase} Addr=${addr:04X} Data=${data:02X} RW={rw} SYNC={sync} VP={vp} IRQ={irq} NMI={nmi} RES={res}",
            cycle = self.cycle,
            phase = phase_char,
            addr = self.pins.addr,
            data = self.pins.data,
            rw = rw_char,
            sync = u8::from(self.pins.sync),
            vp = u8::from(self.pins.vp),
            irq = u8::from(self.pins.irq),
            nmi = u8::from(self.pins.nmi),
            res = u8::from(self.pins.reset),
        );

        if self.show_halfcycles {
            print!(
                " PHI1O={} PHI2O={}",
                u8::from(self.pins.phi1o),
                u8::from(self.pins.phi2o)
            );
        }

        println!();
    }

    /// Reads a message from the serial port and updates `pins` field.
    /// The bridge responds either with a 7-byte pins message (type 2),
    /// or with a 3-byte error message (type 0) when it rejects a request.
    fn read_port(&mut self) {
        let mut header = [0u8; 1];
        self.port
            .read_exact(&mut header)
            .expect("Read error from serial port");

        match header[0] {
            0 => {
                let mut rest = [0u8; 2]; // error code + checksum
                self.port
                    .read_exact(&mut rest)
                    .expect("Read error from serial port");
                panic!("Bridge rejected the message with error code {}", rest[0]);
            }
            2 => {
                let mut buff = [0u8; 7];
                buff[0] = header[0];
                self.port
                    .read_exact(&mut buff[1..])
                    .expect("Read error from serial port");
                let msg = PinsMsg::from_bytes(&buff[..]);
                if SHOW_RAW_DATA {
                    println!("Reading: {}", msg);
                    // print_buff(&buff);
                }
                self.pins = Pins::from(msg.data);
            }
            t => panic!("Unexpected message type: {}", t),
        }
    }

    /// Send the value of `pins` field into to serial port.
    fn write_port(&mut self) {
        let payload: [u8; 5] = self.pins.into();
        let msg = PinsMsg::new(2, payload);
        if SHOW_RAW_DATA {
            print!("Writing: {}", msg);
            // print_buff(&buff);
        }
        self.port
            .write_all(&msg.to_vec())
            .expect("Write error to serial port");
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
