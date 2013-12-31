all : dumbcraft

CC:=gcc
COMMONFLAGS:=  -Os -m32 -flto -g 
CFLAGS:=-I. -I../dumbcraft -I../libs  $(COMMONFLAGS) #-DDEBUG_DUMBCRAFT
LDFLAGS:=-fwhole-program $(COMMONFLAGS)  -Wl,-Map,prog.map -s

dumbcraft : main.o ../dumbcraft/dumbcraft.o ../libs/util10.o ../libs/os_generic.o dumbgame.o
	$(CC) $(LDFLAGS) -o $@ $^

clean :
	rm -rf *.o *~ dumbcraft ../dumbcraft/*.o ../libs/*.o
