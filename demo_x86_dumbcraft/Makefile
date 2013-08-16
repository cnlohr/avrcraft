all : dumbcraft

CFLAGS:=-I. -I../dumbcraft -I../libs -DDEBUG_DUMBCRAFT

dumbcraft : main.o ../dumbcraft/dumbcraft.o ../libs/util10.o ../libs/os_generic.o dumbgame.o
	gcc -o $@ $^ -lpthread

clean :
	rm -rf *.o *~ dumbcraft ../dumbcraft/*.o ../lib/*.o
