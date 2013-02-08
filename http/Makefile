all : program.hex burn program.lst
CC = avr-gcc

#just a random note: use this to benchmark
#curl -s -w "\nResponse time:\t%{time_total}s\n"  -o /dev/null http://192.168.0.142/png.png

SRCS = example.c  \
	../libs/avr_print.c \
	../ipcore/enc424j600.c \
	../ipcore/iparpetc.c \
	../ipcore/enc424j600_asm.S \
	../ipcore/tcp.c \
	../libs/util10.c \
	../http/http.c \
	../mfs/mfs.c \
	myfiles.c
#	../microsd/microsd.c \
#	../microsd/basicfat.c \

myfiles.c : page ../mfs/mfsmaker
	../mfs/mfsmaker $< $@

PROCESSOR=atmega328
PROGRAMCODE=m328
COMMON_FLAGS= -mmcu=$(PROCESSOR) -I. -I.. -I../http -I../microsd -I../libs -I../ipcore -I../dumbcraft -I../mfs
CFLAGS = -std=gnu99 -Os $(COMMON_FLAGS) -DF_CPU=20000000UL -DMUTE_PRINTF #-DNO_HTTP #-Wall  
AS = avr-gcc
ASFLAGS = $(COMMON_FLAGS) -c
EXTRA_LD_FLAGS=-Wl,--relax -Wa,-a,-ad -mcall-prologues 


program.elf : $(SRCS)
	avr-gcc --combine -fwhole-program -I -mmcu=$(PROCESSOR) $(CFLAGS) -o $@ $^ -L /usr/lib64/binutils/avr/2.19.1 $(EXTRA_LD_FLAGS) > program.lst
	avr-size -A $@
	avr-objdump program.elf -t > program.map

program.hex : program.elf
	avr-objcopy -j .text -j .data -O ihex program.elf program.hex 

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

