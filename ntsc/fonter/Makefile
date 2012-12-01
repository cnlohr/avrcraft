all : fonter ntscfont.h

fonter : fonter.o
	gcc -o $@ $^

ntscfont.h : fonter
	./fonter > ntscfont.h

clean :
	rm -rf fonter *.o *~ ntscfont.h
