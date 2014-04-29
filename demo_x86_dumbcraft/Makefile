all : dumbcraft

#for X86
CC:=gcc
COMMONFLAGS:= -Os -m32 -flto -g 

#for MIPS
#CC:=/home/cnlohr/cpptest/ddwrt-toolchains/toolchain-mips_r2_gcc-linaro_uClibc-0.9.32/bin/mips-linux-uclibc-gcc
#COMMONFLAGS:=-Os -flto -g 

CFLAGS:=-Wall -I. -I../dumbcraft -I../libs  $(COMMONFLAGS) #-DDEBUG_DUMBCRAFT
LDFLAGS:=-fwhole-program $(COMMONFLAGS)  -Wl,-Map,prog.map -s

dumbcraft : main.o ../dumbcraft/dumbcraft.o ../libs/util10.o dumbgame.o
	$(CC) $(LDFLAGS) -o $@ $^


clean :
	rm -rf *.o *~ dumbcraft ../dumbcraft/*.o ../libs/*.o
