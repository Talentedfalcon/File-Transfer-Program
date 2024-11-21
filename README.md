## File Transfer using TCP

A simple Peer-to-Peer file transfer application for Linux that uses a server for establishing connection between the clients.

### Usage:
- Use ```make``` to get all the codes compiled in a folder called ```./exec```.
- Either go into the exec folder using ```cd ./exec``` OR you can take the executables and use them elsewhere
- Start the server using ```./_s``` then start the clients using ```./_c <server-ip-address> <server-port-number>``` (by default the server's port number is **12000**)
- Make sure that the ```_peer_client``` and ```_peer_server``` in the same directory as ```_c```

<hr style="height:2px;"></hr>
<div style="
    text-align:center; 
    font-weight:bold; 
    font-size:18px;
">
    OR
</div>
<hr style="height:2px;"></hr>

- Alternatively run ```make test``` to create a ```./testing``` folder where 4 client directories will be initialised along with a server

### Workings:
- **Server Setup**:
    - Accept connections from clients
    - Check for availability of sockets and store their IP addresses and identifiers.
    - Send the list of connected clients to all active clients.
- **Client Setup**:
    - The client will first get it’s IP address and then connect to the main server and send the IP to the server
    - Receives the clients list from the server and prints them out.
- **Client Interaction**:
    - Clients choose a peer from the list and send a connection request message to the server.
    - The server forwards the request and acknowledgment between clients
    - After getting the acknowledgement the requesting client and the requested client will disconnect from the main server
- **Peer Process Execution**:
    - ```execlp()``` is used to spawn peer_server and peer_client processes.
    - The peer_server listens for a connection while the peer_client repeatedly attempts to connect.
    - After successful connection they make 2 threads, one for receiving messages from peer and another to send messages to the peer
- **File Transfer**:
    - A peer wishing to send a file informs the receiver.
    - The sender waits for a confirmation before initiating the transfer.