all : program.hex burn program.lst
CC = avr-gcc

OBJS = test.o avr_print.o enc424j600.o iparpetc.o enc424j600_asm.o tcp.o ../dumbcraft.o ../util10.o http/http.o microsd/microsd.o microsd/basicfat.o
SRCS = test.c avr_print.c enc424j600.c iparpetc.c enc424j600_asm.S tcp.c ../dumbcraft.c ../util10.c http/http.c microsd/microsd.c microsd/basicfat.c

PROCESSOR=atmega328
PROGRAMCODE=m328
COMMON_FLAGS= -mmcu=$(PROCESSOR) -I. -Ihttp -Imicrosd -I..
CFLAGS = -std=gnu99 -Os $(COMMON_FLAGS) -DF_CPU=28636000UL -DMUTE_PRINTF #-DNO_HTTP #-Wall  
AS = avr-gcc
ASFLAGS = $(COMMON_FLAGS) -c
EXTRA_LD_FLAGS=-Wl,--relax -Wa,-a,-ad -mcall-prologues 


program.elf : $(SRCS)
	avr-gcc --combine -fwhole-program -I -mmcu=$(PROCESSOR) $(CFLAGS) -o $@ $^ -L /usr/lib64/binutils/avr/2.19.1 $(EXTRA_LD_FLAGS) > program.lst
	avr-size -A $@
	avr-objdump program.elf -t > program.map

program.hex : program.elf
	avr-objcopy -j .text -j .data -O ihex program.elf program.hex 

#program.lst : $(SRCS)
#	avr-gcc -c -g -Wa,-a,-ad $(CFLAGS) $^ > $@

burn : program.hex
	avrdude -c usbtiny -p $(PROGRAMCODE) -V -U flash:w:program.hex 
#-V 	

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

