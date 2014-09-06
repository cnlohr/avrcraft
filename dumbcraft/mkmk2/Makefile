all : compacketize makecomp makecv final.out final.h 

compypacket.h :

compacketize : compacketize.o
	gcc -o $@ $^

makecomp : makecomp.o
	gcc -o $@ $^ -lz

makecv : makecv.o
	gcc -o $@ $^

%.gz : %.data
	./makecomp < $^ > $@

%.h : %.raw
	./makecv < $^ > $@

%.bin : %.txt
	xxd -r -p $^ $@

%.compacket : %.bin
	./makecomp < $^ > $^.gz
	./compacketize $(shell ./filesize.sh $^) < $^.gz > $@

final.out : packet1.bin packet2.compacket packet3.compacket
	cat $^ > $@

makemap : makemap.o
	gcc -o $@ $^

%.h: %.out
	./makecv < $^ > $@

packet2.bin : makemap
	./makemap > $@

clean :
	rm -rf *.bin *.o makecomp makecv compacketize *.compacket *.gz final.*

#xxd -r -p input.txt output.bin




