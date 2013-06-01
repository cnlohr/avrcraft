all : program.hex burn  #program.lst
CC = avr-gcc

OBJS = main.o ntsc.o video.o
SRCS = main.c ntsc.c video.c

PROCESSOR=atmega328
PROGRAMCODE=m328
CFLAGS = -Wall  -Os -mmcu=$(PROCESSOR) -DF_CPU=20000000UL -I. -Iusbdrv
ASFLAGS = $(CFLAGS) -x assembler-with-cpp
EXTRA_LD_FLAGS= -Wl,--relax -Wa,-a,-ad -mcall-prologues 


program.elf : $(SRCS)
	avr-gcc --combine -fwhole-program -I -mmcu=$(PROCESSOR) $(CFLAGS) -o $@ $^ -L /usr/lib64/binutils/avr/2.19.1 $(EXTRA_LD_FLAGS) > program.lst
	avr-size -A $@
	avr-objdump program.elf -t > program.map

#program.elf : $(OBJS)
#	avr-gcc -I -mmcu=$(PROCESSOR) $(CFLAGS) -Wl,-Map,program.map -o $@ $^ -L /usr/lib64/binutils/avr/2.19.1

program.hex : program.elf
	avr-objcopy -j .text -j .data -O ihex program.elf program.hex 

#program.lst : $(SRCS)
#	avr-gcc --combine -fwhole-program -I -mmcu=$(PROCESSOR) $(CFLAGS) -o $@ $^ -L /usr/lib64/binutils/avr/2.19.1 $(EXTRA_LD_FLAGS) > program.lst
#	avr-gcc -c -g -Wa,-a,-ad $(CFLAGS) $^ > $@

burn : program.hex
	sudo avrdude -c usbtiny -p $(PROGRAMCODE) -V -U flash:w:program.hex


readfuses :
	sudo avrdude -c usbtiny -p $(PROGRAMCODE) -U hfuse:r:high.txt:b -U lfuse:r:low.txt:b

burnfuses :
	sudo avrdude -c usbtiny -p $(PROGRAMCODE) -U lfuse:w:0x7F:m -U hfuse:w:0xDF:m  #don't forget: clock out!

burnfuses_default :
	sudo avrdude -c usbtiny -p $(PROGRAMCODE) -U lfuse:w:0x62:m -U hfuse:w:0xDF:m

clean : 
	rm -f *~ high.txt low.txt program.hex program.map program.elf $(OBJS) *.o usbdrv/*.o usbdrv/*~ program.lst

serial :
	stty -F /dev/ttyUSB0 4800 -parenb -echo -cstopb

