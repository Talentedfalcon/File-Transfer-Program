CC:=gcc
CFLAGS:=-o
server:=server.c
client:=client.c
peer_server:= peer_server.c
peer_client:= peer_client.c

all: 
	$(CC) $(server) $(CFLAGS) s
	$(CC) $(client) $(CFLAGS) c
	$(CC) $(peer_server) $(CFLAGS) peer_server
	$(CC) $(peer_client) $(CFLAGS) peer_client

clean:
	rm s
	rm c
	rm peer_server
	rm peer_client