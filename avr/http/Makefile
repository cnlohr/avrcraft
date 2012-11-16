all : program.hex burn program.lst
CC = avr-gcc

OBJS = ../util10.o ../avr_print.o ../enc424j600.o ../iparpetc.o ../enc424j600_asm.o ../tcp.o http.o test.o microsd/microsd.o microsd/basicfat.o
SRCS = ../util10.c ../avr_print.c ../enc424j600.c ../iparpetc.c ../enc424j600_asm.S ../tcp.c http.c test.c microsd/microsd.c microsd/basicfat.c

PROCESSOR=atmega328
PROGRAMCODE=m328
COMMON_FLAGS= -mmcu=$(PROCESSOR) -I. -Iusbdrv
CFLAGS = -std=gnu99 -Os $(COMMON_FLAGS) -DF_CPU=28000000UL -I. -I.. -Imicrosd # -DMUTE_PRINTF  #-Wall  
AS = avr-gcc
ASFLAGS = $(COMMON_FLAGS) -c

program.elf : $(OBJS)
	avr-gcc -mmcu=$(PROCESSOR) $(CFLAGS) -Wl,-Map,program.map -o $@ $^ -L /usr/lib64/binutils/avr/2.19.1

program.hex : program.elf
	avr-objcopy -j .text -j .data -O ihex program.elf program.hex 

program.lst : $(SRCS)
	avr-gcc -c -g -Wa,-a,-ad $(CFLAGS) $^ > $@

burn : program.hex
	avrdude -c usbtiny -p $(PROGRAMCODE) -V -U flash:w:program.hex


readfuses :
	avrdude -c usbtiny -p $(PROGRAMCODE) -U hfuse:r:high.txt:b -U lfuse:r:low.txt:b

#careful here...
#burnfuses :
#	sudo avrdude -c usbtiny -p $(PROGRAMCODE) -U lfuse:w:0xE6:m -U hfuse:w:0xDF:m
	#use crystal on mega48/168
burnfuses :
	avrdude -c usbtiny -p $(PROGRAMCODE) -U lfuse:w:0xE6:m -U hfuse:w:0xDF:m

clean : 
	rm -f *~ high.txt low.txt program.hex program.map program.elf $(OBJS) *.o usbdrv/*.o usbdrv/*~ program.lst

serial :
	stty -F /dev/ttyUSB0 4800 -parenb -echo -cstopb

