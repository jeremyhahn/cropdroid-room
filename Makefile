ORG := jeremyhahn
PACKAGE := harvest.room
TARGET_OS := linux

.PHONY: flash

default: flash

flash:
	avrdude -c usbasp -p m328p -u -U flash:w:build/Nano/harvest-room.hex

flash-mega:
	avrdude -c usbasp -p m2560 -u -U flash:w:build/Mega/harvest-room.hex