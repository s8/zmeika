PROJECT=apa.io
SOURCES=$(PROJECT).c ../apa.c ../apa.h
MMCU=attiny44
F_CPU = 20000000

CFLAGS=-mmcu=$(MMCU) -Wall -Os -DF_CPU=$(F_CPU)

$(PROJECT).hex: $(PROJECT).out
	avr-objcopy -O ihex $(PROJECT).out $(PROJECT).hex;\
	avr-size --mcu=$(MMCU) --format=avr $(PROJECT).out
 
$(PROJECT).out: $(SOURCES)
	avr-gcc $(CFLAGS) -I./ -o $(PROJECT).out $(SOURCES)
 
program-bsd: $(PROJECT).hex
	avrdude -p t44 -c bsd -U flash:w:$(PROJECT).hex

program-dasa: $(PROJECT).hex
	avrdude -p t44 -P /dev/ttyUSB0 -c dasa -U flash:w:$(PROJECT).hex

program-usbtiny: $(PROJECT).hex
	avrdude -p t44 -P usb -c usbtiny -U flash:w:$(PROJECT).hex

program-usbtiny-fuses: $(PROJECT).hex
	#avrdude -p t44 -P usb -c usbtiny -U lfuse:w:0x5E:m
	avrdude -p t44 -P usb -c usbtiny -U lfuse:w:0x7E:m

program-avrisp2: $(PROJECT).hex
	avrdude -p t44 -P usb -c avrisp2 -U flash:w:$(PROJECT).hex

program-avrisp2-fuses: $(PROJECT).hex
	#avrdude -p t44 -P usb -c avrisp2 -U lfuse:w:0x5E:m
	avrdude -p t44 -P usb -c avrisp2 -U lfuse:w:0x7E:m

program-dragon: $(PROJECT).hex
	avrdude -p t44 -P usb -c dragon_isp -U flash:w:$(PROJECT).hex
