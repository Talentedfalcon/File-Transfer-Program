CC:=gcc
CFLAGS:=-o
server:=main_server.c
client:=client.c
peer_server:= peer_server.c
peer_client:= peer_client.c

code_folder:=./codes/
exec_folder:=./exec/

spacer:=_

all:
	mkdir -p $(exec_folder)
	$(CC) $(code_folder)$(server) $(CFLAGS) $(exec_folder)$(spacer)s
	$(CC) $(code_folder)$(client) $(CFLAGS) $(exec_folder)$(spacer)c
	$(CC) $(code_folder)$(peer_server) $(CFLAGS) $(exec_folder)$(spacer)peer_server
	$(CC) $(code_folder)$(peer_client) $(CFLAGS) $(exec_folder)$(spacer)peer_client

RMFLAGS:=-rf

clean:
	rm $(RMFLAGS) $(exec_folder)