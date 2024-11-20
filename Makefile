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

test_folder:=./testing/
client_folder:=/client

test:
	mkdir -p $(test_folder)
	$(CC) $(code_folder)$(server) $(CFLAGS) $(test_folder)$(spacer)s

	mkdir -p $(test_folder)$(client_folder)1/
	$(CC) $(code_folder)$(client) $(CFLAGS) $(test_folder)$(client_folder)1/$(spacer)c
	$(CC) $(code_folder)$(peer_server) $(CFLAGS) $(test_folder)$(client_folder)1/$(spacer)peer_server
	$(CC) $(code_folder)$(peer_client) $(CFLAGS) $(test_folder)$(client_folder)1/$(spacer)peer_client
	touch $(test_folder)$(client_folder)1/file.txt

	mkdir -p $(test_folder)$(client_folder)2/
	$(CC) $(code_folder)$(client) $(CFLAGS) $(test_folder)$(client_folder)2/$(spacer)c
	$(CC) $(code_folder)$(peer_server) $(CFLAGS) $(test_folder)$(client_folder)2/$(spacer)peer_server
	$(CC) $(code_folder)$(peer_client) $(CFLAGS) $(test_folder)$(client_folder)2/$(spacer)peer_client
	touch $(test_folder)$(client_folder)2/file.txt

	mkdir -p $(test_folder)$(client_folder)3/
	$(CC) $(code_folder)$(client) $(CFLAGS) $(test_folder)$(client_folder)3/$(spacer)c
	$(CC) $(code_folder)$(peer_server) $(CFLAGS) $(test_folder)$(client_folder)3/$(spacer)peer_server
	$(CC) $(code_folder)$(peer_client) $(CFLAGS) $(test_folder)$(client_folder)3/$(spacer)peer_client
	touch $(test_folder)$(client_folder)3/file.txt

clean:
	rm $(RMFLAGS) $(exec_folder)
	rm $(RMFLAGS) $(test_folder)