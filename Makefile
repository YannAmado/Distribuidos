server:
	g++ src/server/main.cpp src/server/server.cpp src/server/channel.cpp src/server/user.cpp src/util.cpp  -lpthread -o server

runServer:
	./server

client:
	g++ client.cpp  -lpthread -o client

runClient:
	./client
