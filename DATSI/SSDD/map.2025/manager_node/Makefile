CFLAGS=-Wall -g -Iutil

all: manager

libutil:
	$(MAKE) -C util

manager.o: common.h common_srv.h manager.h srv_addr_arr.h
common.o: common.h
common_srv.o: common_srv.h

manager: manager.o common.o common_srv.o srv_addr_arr.o
	$(CC) -o $@ $< common.o common_srv.o srv_addr_arr.o -lpthread

clean:
	rm -f *.o manager
