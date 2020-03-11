ORG := jeremyhahn
PACKAGE := cropdroid.room
TARGET_OS := linux

AVRDUDE=/home/jhahn/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino14/bin/avrdude
CONF=-C/home/jhahn/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino14/etc/avrdude.conf

.PHONY: flash flash-nano flash-mega bootloader

default: flash

#flash:
#	$(AVRDUDE) -c usbasp -p m328p -u -U flash:w:build/Nano/cropdroid-room.hex

flash-usbasp:
	avrdude -c usbasp -p m328p -u -U flash:w:build/Nano/cropdroid-room.hex

flash-nano:
	avrdude -v -patmega328p -carduino -P/dev/ttyUSB0 -b115200 -D -U flash:w:build/Nano/cropdroid-room.hex

flash-mega:
	$(AVRDUDE) -c usbasp -p m2560 -u -U flash:w:build/Mega/cropdroid-room.hex

bootloader:
	$(AVRDUDE) $(CONF) -v -patmega328p -cusbasp -Pusb -e -Ulock:w:0x3F:m -Uefuse:w:0xFD:m -Uhfuse:w:0xDA:m -Ulfuse:w:0xFF:m
	$(AVRDUDE) $(CONF) -v -patmega328p -cusbasp -Pusb -Uflash:w:bootloader/optiboot_atmega328.hex:i -Ulock:w:0x0F:m
