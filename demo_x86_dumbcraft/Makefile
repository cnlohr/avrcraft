all : dumbcraft

CFLAGS:=-I. -I../dumbcraft -I../libs

dumbcraft : main.o ../dumbcraft/dumbcraft.o ../libs/util10.o ../libs/os_generic.o
	gcc -o $@ $^ -lpthread

clean :
	rm -rf *.o *~ dumbcraft
