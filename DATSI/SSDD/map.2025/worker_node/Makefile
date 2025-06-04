CFLAGS=-Wall -g

all: worker

worker.o: common.h common_srv.h common_cln.h worker.h manager.h
common.o: common.h
common_cln.o: common_cln.h
common_srv.o: common_srv.h

worker: worker.o common.o common_srv.o common_cln.o
	$(CC) -o $@ $< common.o common_srv.o common_cln.o

clean:
	rm -f *.o worker
