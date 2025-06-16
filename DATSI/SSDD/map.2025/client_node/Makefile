CFLAGS=-Wall -g -fPIC

all: map

mgfs.o: common.h common_cln.h manager.h worker.h

map: map.o common.o common_cln.o
	$(CC) -o $@ $< common.o common_cln.o

clean:
	rm -f *.o map


