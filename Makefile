FQBN ?= teensy:avr:teensy41
PORT ?= usb:8443000

build:
	arduino-cli compile -b $(FQBN) .

debug:
	arduino-cli compile -b $(FQBN) --build-property "build.extra_flags=\"-DDEBUG_TEENSY_COM_BRIDGE\"" .

upload:
	arduino-cli upload -b $(FQBN) --port $(PORT) .


