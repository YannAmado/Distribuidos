/*#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>

#define MAX_STR 50
#define BYTES_PER_MSG 4096


using namespace std;

struct servidor{
	int id;
	string name;
	int socket;
	thread th;
    int adm;
};

vector <servidor> clients;
int contagem_cliente = 0;
mutex cout_mtx, clients_mtx;

// configura o nome do cliente
void set_name(int id, char name[]){
	for(int i=0; i<clients.size(); i++)
	    if(clients[i].id==id)	
			clients[i].name=string(name);
}

// Sincroniza os terminais
void terminal_compartilhado(string str, bool endLine=true){	
	lock_guard<mutex> guard(cout_mtx);
	cout<<str;
	if(endLine)
		cout<<endl;
}

// Manda uma string para todos os usuarios
void broadcast_message(string message, int sender_id){
	char temp[BYTES_PER_MSG] = {'\0'};
	strcpy(temp,message.c_str());

	for(int i=0; i<clients.size(); i++)
	    if(clients[i].id!=sender_id)
			send(clients[i].socket,temp,sizeof(temp),0);
}

// Manda um numero para todos os usuarios (configuração nescessaria para separar eles)
void broadcast_message(int num, int sender_id){
	for(int i=0; i<clients.size(); i++)
		if(clients[i].id!=sender_id)
		    send(clients[i].socket,&num,sizeof(num),0);
}



void end_connection(int id){
	for(int i=0; i<clients.size(); i++)
		if(clients[i].id==id){
			lock_guard<mutex> guard(clients_mtx);
			clients[i].th.detach();
			clients.erase(clients.begin()+i);
			close(clients[i].socket);
			break;
		}			
}

void configura_cliente(int client_socket, int id){
	char name[MAX_STR],str[BYTES_PER_MSG];
    string id_char;
    id_char = to_string(id);
	
    cout << "\nCAST ID PARA CHAR INT:" << id << "CHAR :"<< id_char << "\n";
	recv(client_socket,name,sizeof(name),0);
	set_name(id,name);	

	// Display welcome message
	string bem_vindo=string("-")+string(name)+string(" entrou!");
	broadcast_message("#NULL",id);	
	broadcast_message(id,id);								
	broadcast_message(bem_vindo,id);	
	terminal_compartilhado(id_char + bem_vindo);

	while(1){
       // memset(str, 0, strlen(str));
		recv(client_socket,str,sizeof(str),0);
        cout << "teste: " << str <<"\n";
		
       //if(bytes_received<=0)
			//return;
		
        if(strcmp(str,"#exit")==0){
			// Display leaving message
			string message=string("-")+string(name)+string(" saiu do chat!");		
			broadcast_message("#NULL",id);			
			broadcast_message(id,id);						
			broadcast_message(message,id);
			terminal_compartilhado(id_char +message);
			end_connection(id);							
			return;
		}

        broadcast_message(string(name),id);
		broadcast_message(id,id);		
		broadcast_message(string(str),id);



        string message=string(id_char)+string("-")+string(name)+string(": ");
		terminal_compartilhado(message + str);
		
	}	
}

int main(){
    struct sockaddr_in client;
	int client_socket;
	int server_socket = socket(AF_INET,SOCK_STREAM,0);
    
    if (server_socket == -1) {
        cout << "Erro: server_socket, reinicie o servidor" << endl;
        return EXIT_FAILURE;
	}
		

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(9002); 
	server.sin_addr.s_addr = INADDR_ANY;

    unsigned int len = sizeof(sockaddr_in);
	
	if((bind(server_socket,(struct sockaddr *)&server,sizeof(struct sockaddr_in)))==-1){
		perror("bind error: ");
		exit(0);
	}

	if((listen(server_socket,8))==-1){
		perror("listen error: ");
		exit(0);
	}

	while(1){
		client_socket = accept(server_socket,(struct sockaddr *)&client,&len);

        if(client_socket == -1){
            printf("Erro ao se conectar\n\n");
            exit(0);
        }else
            printf("Conectado ao cliente\n");

		contagem_cliente++;
		thread t(configura_cliente,client_socket,contagem_cliente);
		lock_guard<mutex> guard(clients_mtx);
		clients.push_back({contagem_cliente, string("Anonymous"),client_socket,(move(t))});
	}

	for(int i = 0; i < clients.size(); i++)
		if(clients[i].th.joinable())
			clients[i].th.join();
	

	close(server_socket);
}*/

#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>

#define MAX_STR 50
#define BYTES_PER_MSG 4096


using namespace std;

struct servidor{
	int id;
	string name;
	int socket;
	thread th;
    int adm;
};

vector <servidor> clients;
int contagem_cliente = 0;
mutex cout_mtx, clients_mtx;

// configura o nome do cliente
void set_name(int id, char name[]){
	for(int i=0; i<clients.size(); i++)
	    if(clients[i].id==id)	
			clients[i].name=string(name);
}

// Sincroniza os terminais
void terminal_compartilhado(string str, bool endLine=true){	
	lock_guard<mutex> guard(cout_mtx);
	cout<<str;
	if(endLine)
		cout<<endl;
}

// Manda uma string para todos os usuarios
void broadcast_message(string message, int sender_id){
	char temp[BYTES_PER_MSG] = {'\0'};
    //cout << "Chegou broadcast_message" <<endl;
	strcpy(temp,message.c_str());

    //cout << "Chegou copia" <<endl;
	for(int i=0; i<clients.size(); i++)
	    if(clients[i].id!=sender_id)
			send(clients[i].socket,temp,sizeof(temp),0);
}

// Manda um numero para todos os usuarios (configuração nescessaria para separar eles)
void broadcast_message(int num, int sender_id){
	for(int i=0; i<clients.size(); i++)
		if(clients[i].id!=sender_id)
		    send(clients[i].socket,&num,sizeof(num),0);
}

void end_connection(int id){
	for(int i=0; i<clients.size(); i++)
		if(clients[i].id==id){
			lock_guard<mutex> guard(clients_mtx);
			clients[i].th.detach();
			clients.erase(clients.begin()+i);
			close(clients[i].socket);
			break;
		}			
}

void configura_cliente(int client_socket, int id){
	char name[MAX_STR],str[BYTES_PER_MSG];
    string id_char;
    id_char = to_string(id);

    //cout << "\nCAST ID PARA CHAR INT:" << id << "CHAR :"<< id_char << "\n";
	recv(client_socket,name,sizeof(name),0);
	set_name(id,name);	

	// Display welcome message
	string bem_vindo=string("-")+string(name)+string(" entrou!");
	broadcast_message("#NULL",id);	
	broadcast_message(id,id);								
	broadcast_message(bem_vindo,id);	
	terminal_compartilhado(id_char + bem_vindo);
	
	while(1){
        memset(str, 0, strlen(str));
		int bytes_received = recv(client_socket,str,sizeof(str),0);
        //cout << "teste: " << str <<endl;
		
        if(bytes_received<=0)
			return;
		
        //cout << "Chegou aqui" <<endl;
        if(strcmp(str,"#exit")==0){
			// Display leaving message
			string message=string("-")+string(name)+string(" saiu do chat!");		
			broadcast_message("#NULL",id);			
			broadcast_message(id,id);						
			broadcast_message(message,id);
			terminal_compartilhado(id_char +message);
			end_connection(id);							
			return;
		}
        //cout << "Chegou aqui 2" <<endl;
        broadcast_message(string(name),id);
        //cout << "Chegou aqui 4" <<endl;				
		broadcast_message(id,id);		
        //cout << "Chegou aqui 5" <<endl;
		broadcast_message(string(str),id);

        //cout << "Chegou aqui 3" <<endl;
        string message=string("-")+string(name)+string(":")+string(str);
        cout << "MESSAGE: " << message  <<"\n";
		terminal_compartilhado(id_char + message);	
	}	
}

int main(){
    struct sockaddr_in client;
	int client_socket;
	int server_socket = socket(AF_INET,SOCK_STREAM,0);
    
    if (server_socket == -1) {
        cout << "Erro: server_socket, reinicie o servidor" << endl;
        return EXIT_FAILURE;
	}
		

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(9002); 
	server.sin_addr.s_addr = INADDR_ANY;

    unsigned int len = sizeof(sockaddr_in);
	
	if((bind(server_socket,(struct sockaddr *)&server,sizeof(struct sockaddr_in)))==-1){
		perror("bind error: ");
		exit(0);
	}

	if((listen(server_socket,8))==-1){
		perror("listen error: ");
		exit(0);
	}

	while(1){
		client_socket = accept(server_socket,(struct sockaddr *)&client,&len);

        if(client_socket == -1){
            printf("Erro ao se conectar\n\n");
            exit(0);
        }else
            printf("Conectado ao cliente\n");

		contagem_cliente++;
		thread t(configura_cliente,client_socket,contagem_cliente);
		lock_guard<mutex> guard(clients_mtx);
		clients.push_back({contagem_cliente, string("Anonymous"),client_socket,(move(t))});
	}

	for(int i = 0; i < clients.size(); i++)
		if(clients[i].th.joinable())
			clients[i].th.join();
	

	close(server_socket);
}