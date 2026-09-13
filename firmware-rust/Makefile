HOST := $(shell rustc -vV | sed -n 's/host: //p')
TARGET := thumbv7em-none-eabihf
ELF := target/$(TARGET)/release/teensy-6502-bridge
OBJCOPY := $(shell find $(shell rustc --print sysroot)/lib/rustlib/*/bin -name llvm-objcopy 2>/dev/null | head -1)

.PHONY: build hex flash test clean

build:
	cargo build --release

hex: build
	$(OBJCOPY) -O ihex $(ELF) teensy-6502-bridge.hex

# Requires teensy-loader-cli (a native macOS/Linux build). If the Rust
# firmware is already running, it reboots into the bootloader when the
# port is opened at 134 baud - otherwise press the button when prompted.
flash: hex
	teensy-loader-cli --mcu=TEENSY41 -w -v teensy-6502-bridge.hex

test:
	cargo test --lib --target $(HOST)

clean:
	cargo clean
	rm -f teensy-6502-bridge.hex
