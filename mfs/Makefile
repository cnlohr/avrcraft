all : mfsmaker test_fs.c

mfsmaker : mfsmaker.o
	gcc -o $@ $^

test_fs.c : test mfsmaker
	./mfsmaker $< $@

clean :
	rm -rf *.o *~ test_fs.c mfsmaker



