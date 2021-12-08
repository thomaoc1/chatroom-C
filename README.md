# chatroom-C

1. client sends package [message size (size_t), timestamp (time_t), message (char *)] to server
2. server receives package and sends to all clients
3. clients receive package and display it on screen

First package sent before every message is the client username

OR 

Every message sent is sent with the prefix "username: message"


1. Send function

2. Receive function 