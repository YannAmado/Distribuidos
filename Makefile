CC=g++
CFLAGS= -Wall -c

server: server_main.o server.o user.o channel.o util.o
	$(CC) server_main.o server.o channel.o user.o util.o -o server

server_main.o: src/server/main.cpp
	$(CC) $(CFLAGS) src/server/main.cpp -o server_main.o

user.o: src/server/user.cpp
	$(CC) $(CFLAGS) src/server/user.cpp

channel.o: src/server/channel.cpp
	$(CC) $(CFLAGS) src/server/channel.cpp -lpthread

server.o: src/server/server.cpp
	$(CC) $(CFLAGS) src/server/server.cpp -lpthread

util.o: src/util.cpp
	$(CC) $(CFLAGS) src/util.cpp

client_main.o: src/client/main.cpp
	$(CC) $(CFLAGS) src/client/main.cpp -o client_main.o

client.o: src/client/client.cpp
	$(CC) $(CFLAGS) src/client/client.cpp -lpthread

client: client_main.o client.o util.o
	$(CC) client_main.o client.o util.o  -o client

runServer:
	./server

runClient:
	./client

clean:
	rm -rf *.o
