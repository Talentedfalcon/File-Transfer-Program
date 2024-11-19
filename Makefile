CC:=gcc
CFLAGS:=-o
server:=server.c
client:=client.c


all: 
	$(CC) $(server) $(CFLAGS) s
	$(CC) $(client) $(CFLAGS) c

clean:
	rm s
	rm c