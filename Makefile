server: server.o
	clear
	stty icanon
	g++ server.cpp  -lpthread -o server

runServer:
	./server

client: client.o
	clear
	stty icanon
	g++ client.cpp  -lpthread -o client

runClient:
	./client
