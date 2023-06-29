server: server.o 
	stty -icanon
	g++ server.cpp -lpthread -o server

runServer:
	./server

client: client.o
	stty -icanon
	g++ client.cpp -lpthread -o client

runClient:
	./client

