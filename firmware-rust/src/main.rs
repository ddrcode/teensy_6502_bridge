//! W65C02-Teensy 4.1 bridge firmware (Rust edition).
//!
//! A bare-metal implementation of the same serial protocol as the C++
//! firmware (see `docs/protocol.md`): 7-byte framed messages over USB CDC,
//! one request-response pair per clock half-cycle, with checksum validation,
//! error replies, and the data bus driven only during the high clock phase
//! of read cycles.
//!
//! Extra: opening the serial port at 134 baud reboots the board into the
//! HalfKay bootloader (same convention as Teensyduino), so reflashing
//! doesn't require pressing the button.

#![no_std]
#![no_main]

use teensy4_panic as _;

use teensy4_bsp as bsp;

use bsp::board;
use bsp::hal::gpio::{Input, Output, Port};
use bsp::hal::iomuxc;
use bsp::pins::t41::Pins;
use bsp::usbd::{BusAdapter, EndpointMemory, EndpointState};

use static_cell::StaticCell;
use usb_device::bus::UsbBusAllocator;
use usb_device::device::{StringDescriptors, UsbDeviceState};
use usb_device::prelude::*;
use usb_device::UsbError;
use usbd_serial::SerialPort;

use bridge::payload::{self, bits, BUFF_SIZE};
use bridge::protocol::{
    checksum, error_msg, Accumulator, Event, ERR_INVALID_CHECKSUM, ERR_INVALID_MSG_TYPE, MSG_PINS,
    PINS_MSG_SIZE,
};

static EP_MEMORY: EndpointMemory<2048> = EndpointMemory::new();
static EP_STATE: EndpointState<8> = EndpointState::new();
static USB_ALLOC: StaticCell<UsbBusAllocator<BusAdapter>> = StaticCell::new();

type Device<'a> = UsbDevice<'a, BusAdapter>;
type Serial<'a> = SerialPort<'a, BusAdapter>;

//--------------------------------------------------------------------------
// GPIO layer

/// Creates an output GPIO, panicking on a pin/port mismatch (a build-time
/// mapping bug - the panic handler blinks the LED).
///
/// SION keeps the pad's input path alive so the driven level can be read
/// back through PSR - `sample()` and the PHI2 gate in `handle_cycle()`
/// depend on it. (`alternate()` preserves the bit, so setting it before
/// `Port::output` is safe.)
fn out<P, const N: u8>(port: &mut Port, mut pin: P) -> Output
where
    P: iomuxc::gpio::Pin<N>,
{
    iomuxc::set_sion(&mut pin);
    match port.output(pin) {
        Ok(output) => output,
        Err(_) => panic!(),
    }
}

/// Creates an input GPIO, panicking on a pin/port mismatch.
fn inp<P, const N: u8>(port: &mut Port, pin: P) -> Input
where
    P: iomuxc::gpio::Pin<N>,
{
    match port.input(pin) {
        Ok(input) => input,
        Err(_) => panic!(),
    }
}

/// One data bus line. Holds two aliased handles: a DR writer (used only
/// while the bus is driven) and a PSR reader (valid in both directions).
/// The actual direction is flipped via [`CpuPins::set_data_direction`].
struct DataPin {
    port_no: u8,
    offset: u32,
    writer: Output,
    sample: Input,
}

fn data_pin<P, const N: u8>(port: &mut Port, pin: P) -> DataPin
where
    P: iomuxc::gpio::Pin<N>,
{
    let offset = P::OFFSET;
    // `out` muxes the pad (with SION, so the bus can be sampled even while
    // driven) and briefly sets GDIR to out...
    let writer = out(port, pin);
    // ...and this reverts GDIR so the bus starts released (input).
    let sample = Input::without_pin(port, offset);
    DataPin {
        port_no: N,
        offset,
        writer,
        sample,
    }
}

/// All CPU-facing GPIOs, following the default `PINS_MAP` of the C++
/// firmware (see the pinout table in the README).
struct CpuPins {
    gpio1: Port,
    gpio2: Port,
    gpio3: Port,
    gpio4: Port,

    // Teensy outputs (CPU inputs)
    ready: Output,
    irq: Output,
    nmi: Output,
    be: Output,
    so: Output,
    reset: Output,
    phi2: Output,

    // Teensy inputs (CPU outputs)
    vp: Input,
    phi1o: Input,
    ml: Input,
    sync: Input,
    rw: Input,
    phi2o: Input,
    addr: [Input; 16],

    data: [DataPin; 8],
    data_out: bool,
}

impl CpuPins {
    fn new(
        mut gpio1: Port,
        mut gpio2: Port,
        mut gpio3: Port,
        mut gpio4: Port,
        pins: Pins,
    ) -> Self {
        let g1 = &mut gpio1;
        let g2 = &mut gpio2;
        let g3 = &mut gpio3;
        let g4 = &mut gpio4;

        let cpu = Self {
            ready: out(g1, pins.p21),
            irq: out(g4, pins.p3),
            nmi: out(g4, pins.p5),
            be: out(g1, pins.p19),
            so: out(g1, pins.p20),
            reset: out(g1, pins.p23),
            phi2: out(g2, pins.p13),

            vp: inp(g1, pins.p0),
            phi1o: inp(g4, pins.p2),
            ml: inp(g4, pins.p4),
            sync: inp(g2, pins.p6),
            rw: inp(g1, pins.p17),
            phi2o: inp(g1, pins.p22),

            // A0..A15
            addr: [
                inp(g2, pins.p7),
                inp(g2, pins.p8),
                inp(g2, pins.p9),
                inp(g2, pins.p11),
                inp(g2, pins.p12),
                inp(g1, pins.p24),
                inp(g1, pins.p25),
                inp(g3, pins.p28),
                inp(g4, pins.p29),
                inp(g3, pins.p30),
                inp(g3, pins.p31),
                inp(g2, pins.p32),
                inp(g4, pins.p33),
                inp(g2, pins.p34),
                inp(g2, pins.p35),
                inp(g2, pins.p36),
            ],

            // D0..D7
            data: [
                data_pin(g1, pins.p16),
                data_pin(g1, pins.p15),
                data_pin(g1, pins.p14),
                data_pin(g1, pins.p41),
                data_pin(g1, pins.p40),
                data_pin(g1, pins.p39),
                data_pin(g1, pins.p38),
                data_pin(g2, pins.p37),
            ],
            data_out: false,

            gpio1,
            gpio2,
            gpio3,
            gpio4,
        };

        // Safe defaults: all active-low control signals deasserted
        cpu.ready.set();
        cpu.irq.set();
        cpu.nmi.set();
        cpu.be.set();
        cpu.so.set();
        cpu
    }

    /// Holds the CPU in reset for a while, then releases it (the host
    /// performs its own protocol-level reset sequence afterwards).
    fn reset_cpu(&mut self) {
        self.reset.clear();
        delay_ms(100);
        delay_ms(100);
        self.reset.set();
        delay_ms(100);
    }

    fn port(&mut self, no: u8) -> &mut Port {
        match no {
            1 => &mut self.gpio1,
            2 => &mut self.gpio2,
            3 => &mut self.gpio3,
            _ => &mut self.gpio4,
        }
    }

    /// Drives or releases the data bus (GDIR flip on all 8 lines).
    fn set_data_direction(&mut self, drive: bool) {
        if drive == self.data_out {
            return;
        }
        self.data_out = drive;
        for i in 0..8 {
            let (no, offset) = (self.data[i].port_no, self.data[i].offset);
            let port = self.port(no);
            if drive {
                let _ = Output::without_pin(port, offset);
            } else {
                let _ = Input::without_pin(port, offset);
            }
        }
    }

    /// Applies the host-controlled pins from an incoming payload.
    /// Mirrors the C++ `set_pins_state`.
    fn apply(&mut self, p: &[u8; BUFF_SIZE]) {
        set_level(&self.irq, payload::get_bit(p, bits::IRQ));
        set_level(&self.nmi, payload::get_bit(p, bits::NMI));
        set_level(&self.be, payload::get_bit(p, bits::BE));
        set_level(&self.phi2, payload::get_bit(p, bits::PHI2));
        set_level(&self.so, payload::get_bit(p, bits::SO));
        set_level(&self.reset, payload::get_bit(p, bits::RES));

        if self.rw.is_set() {
            // latch data values; they appear on the bus once driven
            for i in 0..8 {
                set_level(&self.data[i].writer, payload::get_bit(p, payload::data_bit(i as u8)));
            }
        }
    }

    /// Drives the data bus only during the high clock phase of read cycles
    /// (like a memory's output enable gated with RW and PHI2). Mirrors the
    /// C++ `handle_cycle`.
    fn handle_cycle(&mut self) {
        let drive = self.rw.is_set() && self.phi2.is_pad_high();
        self.set_data_direction(drive);
    }

    /// Samples all 40 CPU pins into a payload. Mirrors the C++
    /// `get_pins_state` (VDD/VSS/NC bits stay zero).
    fn sample(&self) -> [u8; BUFF_SIZE] {
        let mut p = [0u8; BUFF_SIZE];
        payload::set_bit(&mut p, bits::VP, self.vp.is_set());
        payload::set_bit(&mut p, bits::RDY, self.ready.is_pad_high());
        payload::set_bit(&mut p, bits::PHI1O, self.phi1o.is_set());
        payload::set_bit(&mut p, bits::IRQ, self.irq.is_pad_high());
        payload::set_bit(&mut p, bits::ML, self.ml.is_set());
        payload::set_bit(&mut p, bits::NMI, self.nmi.is_pad_high());
        payload::set_bit(&mut p, bits::SYNC, self.sync.is_set());
        for i in 0..16 {
            payload::set_bit(&mut p, payload::addr_bit(i), self.addr[i as usize].is_set());
        }
        for i in 0..8 {
            payload::set_bit(&mut p, payload::data_bit(i), self.data[i as usize].sample.is_set());
        }
        payload::set_bit(&mut p, bits::RW, self.rw.is_set());
        payload::set_bit(&mut p, bits::BE, self.be.is_pad_high());
        payload::set_bit(&mut p, bits::PHI2, self.phi2.is_pad_high());
        payload::set_bit(&mut p, bits::SO, self.so.is_pad_high());
        payload::set_bit(&mut p, bits::PHI2O, self.phi2o.is_set());
        payload::set_bit(&mut p, bits::RES, self.reset.is_pad_high());
        p
    }
}

#[inline]
fn set_level(pin: &Output, high: bool) {
    if high {
        pin.set();
    } else {
        pin.clear();
    }
}

fn delay_ms(ms: u32) {
    cortex_m::asm::delay(ms * (board::ARM_FREQUENCY / 1_000));
}

//--------------------------------------------------------------------------
// USB plumbing

/// Writes a complete message and flushes it to the host, pumping the USB
/// device as needed (the equivalent of the C++ `Serial.send_now()`).
fn send_msg(device: &mut Device, serial: &mut Serial, bytes: &[u8]) {
    let mut written = 0;
    while written < bytes.len() {
        match serial.write(&bytes[written..]) {
            Ok(n) => written += n,
            Err(UsbError::WouldBlock) => {
                device.poll(&mut [serial]);
            }
            Err(_) => return,
        }
    }
    loop {
        match serial.flush() {
            Err(UsbError::WouldBlock) => {
                device.poll(&mut [serial]);
            }
            _ => break,
        }
    }
}

/// Reboots into the HalfKay bootloader (the Teensyduino convention).
fn reboot_to_bootloader() -> ! {
    cortex_m::interrupt::disable();
    unsafe { core::arch::asm!("bkpt #251") };
    loop {
        cortex_m::asm::nop();
    }
}

//--------------------------------------------------------------------------
// Entry point

#[bsp::rt::entry]
fn main() -> ! {
    let board::Resources {
        gpio1,
        gpio2,
        gpio3,
        gpio4,
        pins,
        usb,
        ..
    } = board::t41(board::instances());

    let mut cpu = CpuPins::new(gpio1, gpio2, gpio3, gpio4, pins);
    cpu.reset_cpu();

    let bus = BusAdapter::new(usb, &EP_MEMORY, &EP_STATE); // high-speed by default
    bus.set_interrupts(false); // polling mode
    let alloc = USB_ALLOC.init(UsbBusAllocator::new(bus));
    let mut serial = SerialPort::new(alloc);
    let mut device = UsbDeviceBuilder::new(alloc, UsbVidPid(0x16C0, 0x0483))
        .device_class(usbd_serial::USB_CLASS_CDC)
        .max_packet_size_0(64)
        .unwrap()
        .strings(&[StringDescriptors::default()
            .manufacturer("ddrcode")
            .product("W65C02 Bridge (Rust)")
            .serial_number("6502-RS-2")])
        .unwrap()
        .build();

    let mut configured = false;
    let mut acc = Accumulator::new();

    loop {
        device.poll(&mut [&mut serial]);

        if device.state() != UsbDeviceState::Configured {
            configured = false;
            acc.clear();
            continue;
        }
        if !configured {
            // finalize endpoint configuration (imxrt-usbd requirement)
            device.bus().configure();
            configured = true;
        }

        if serial.line_coding().data_rate() == 134 {
            reboot_to_bootloader();
        }

        if !serial.dtr() {
            acc.clear();
            continue;
        }

        let mut buf = [0u8; 64];
        let count = match serial.read(&mut buf) {
            Ok(n) => n,
            Err(_) => 0,
        };

        for &byte in &buf[..count] {
            match acc.push(byte) {
                Event::Pending => {}
                Event::UnknownType => {
                    send_msg(&mut device, &mut serial, &error_msg(ERR_INVALID_MSG_TYPE));
                }
                Event::Complete {
                    msg_type,
                    payload,
                    checksum_ok,
                } => {
                    if msg_type != MSG_PINS {
                        send_msg(&mut device, &mut serial, &error_msg(ERR_INVALID_MSG_TYPE));
                    } else if !checksum_ok {
                        send_msg(&mut device, &mut serial, &error_msg(ERR_INVALID_CHECKSUM));
                    } else {
                        cpu.apply(&payload);
                        cpu.handle_cycle();
                        let sampled = cpu.sample();
                        let mut reply = [0u8; PINS_MSG_SIZE];
                        reply[0] = MSG_PINS;
                        reply[1..6].copy_from_slice(&sampled);
                        reply[6] = checksum(&reply[..6]);
                        send_msg(&mut device, &mut serial, &reply);
                    }
                }
            }
        }
    }
}
