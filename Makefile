ORG := jeremyhahn
PACKAGE := harvest.room
TARGET_OS := linux

.PHONY: flash flash-mega

default: flash

flash:
	avrdude -c usbasp -p m328p -u -U flash:w:build/Nano/harvest-room.hex

flash-nano:
	avrdude -v -patmega328p -carduino -P/dev/ttyUSB0 -b115200 -D -U flash:w:build/Nano/harvest-room.hex

flash-mega:
	avrdude -c usbasp -p m2560 -u -U flash:w:build/Mega/harvest-room.hex