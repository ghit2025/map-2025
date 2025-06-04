CFLAGS=-Wall

PROGRS=client server_seq  server_thr sender receiver select

all:  $(PROGRS)

client.o sender.o select.o common_cln.o: common_cln.h
server_seq.o server_thr receiver.o: common_srv.h

client: client.o common_cln.o
	$(CC) -o $@ $< common_cln.o

server_seq: server_seq.o common_srv.o
	$(CC) -o $@ $< common_srv.o

server_thr: server_thr.o common_srv.o
	$(CC) -o $@ $< common_srv.o -lpthread

sender: sender.o common_cln.o
	$(CC) -o $@ $< common_cln.o

receiver: receiver.o common_srv.o
	$(CC) -o $@ $< common_srv.o

select: select.o common_cln.o
	$(CC) -o $@ $< common_cln.o

clean:
	rm -f $(PROGRS) *.o
