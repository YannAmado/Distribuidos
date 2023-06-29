#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <signal.h>
#include <mutex>

#define MAX_STR 50
#define BYTES_PER_MSG 5

using namespace std;

bool exit_flag=false;
thread t_send, t_recv;
int client_socket;

void blockCrtl_C(int signal) {
	cout<<("Ctrl + c nao esta disponivel")<<endl;
	cout<<("Se deseja sair do chat utilize o comando /quit ou CTRL+D")<<endl;
}

char* create_block(char* msg, int current_block){
    //+1 pra incluir o '\0'
    char* msg_block = (char*)malloc(sizeof(char)*(BYTES_PER_MSG+1));
    int i = 1;
    int pos = current_block*BYTES_PER_MSG;
    msg_block[0] = msg[pos];
    while(msg[pos + i] != '\n' && msg[pos + i] != '\r' && msg[pos + i] != '\0' && i%BYTES_PER_MSG != 0){
        msg_block[i] = msg[pos + i];
        i++;
    }
    msg_block[i] = '\0';
    return msg_block;
}


void send_message(int client_socket){
    
    //int* client_socket_ptr = (int *) client_socket;
    char* server_message = NULL;
    char* block = NULL;
    while(1){
       // cout<<"Voce : ";
        server_message = NULL;
        block = NULL;
        
        cin >> server_message;
        //scanf(" %m[^\n\r]", &server_message);
        cout << "SERVER MESSAGE: "<< server_message << "\n";
        int message_size = 0;
        int n_blocks = 0;

        while(server_message[message_size] != '\0') {
            if(message_size%BYTES_PER_MSG == 0){
                n_blocks++;
            }
            message_size++;
        }

      
        //enviando os blocos
        int j = 0;
        //send(*client_socket_ptr, server_message, message_size, 0);
        for(int i = 0; i < n_blocks; i++){
            block = create_block(server_message, i);
            while(block[j] != '\0') j++;
            cout << "J: "<< j << "strlen " <<strlen(block) << "\n";
            //cout<<"BLOCO\n" << block <<"\n";
            send(client_socket, block,j,0);
            if((strncmp(block,"/quit", 5) == 0) || (int)block[0] == 4){
                printf("Saindo do Servidor");
                exit_flag=true;
                t_recv.detach();	
                close(client_socket);
                return;
            }
            free(block);
        }

           

        cout<<"n_blocks " << n_blocks;
        cout<<" - message_size " <<message_size <<"\n";
        free(server_message);
    }

	/*while(1){
		cout<<"Voce : ";
		char str[BYTES_PER_MSG];
		
        cin.getline(str,BYTES_PER_MSG);
        
		send(client_socket,str,sizeof(str),0);
		
        if((strncmp(str,"/quit", 5) == 0) || (int)str[0] == 4){
            printf("Saindo do Servidor");
			exit_flag=true;
			t_recv.detach();	
			close(client_socket);
			return;
		}	
	}	*/
}


void recv_message(int client_socket){
	std::string str;
    while(1){
		if(exit_flag)
			return;
		
        char name[MAX_STR], str[BYTES_PER_MSG];
		int maisDeUmUsuario;
		int bytes_received=recv(client_socket,name,sizeof(name),0);
		
        if(bytes_received<=0)
			continue;
		
        recv(client_socket,&maisDeUmUsuario,sizeof(maisDeUmUsuario),0);		
        recv(client_socket,str,sizeof(str),0);
		
        if(strcmp(name,"#NULL")!=0)
			cout<<name<<": "<<str<<endl;
		else
			cout<<str<<endl;
        cout<<"Voce: ";
		fflush(stdout);
	}	
}


int main(){
    char username[MAX_STR] = {'\0'};
    char conexao [7] = {'\0'};
    int op = 0;
	client_socket = socket(AF_INET,SOCK_STREAM,0);
	
	struct sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_port = htons(9002); 
	client.sin_addr.s_addr = INADDR_ANY;

    //ignorar ctrl+c
	signal(SIGINT, blockCrtl_C);
    signal(SIGQUIT,blockCrtl_C);

    while(op == 0){
        cout<<"\nDigite /connect se deseja se estabelecr uma conexao com o servidor"<<endl;
        cin >> conexao;

        if(strcmp(conexao,"/connect") == 0){
            int conectando = connect(client_socket,(struct sockaddr *)&client,sizeof(struct sockaddr_in));
            if (conectando == -1) {
                cout << "Erro: connectando, reinicie o servidor" << endl;
                return EXIT_FAILURE;
	        }

            cout<<"\nPara determianr o seu nome de usuario digite: /nickname seuNomeUsuario"<<endl;
            
            while(1){
                char nickname[9] = {'\0'};
                cin >> nickname;

                if((strncmp(nickname,"/quit", 5) == 0) || (int)nickname[0] == 4){
                    cout << "Saindo do Servidor" << endl;
                    t_recv.detach();	
			        close(client_socket);
			        exit(0);
  	            }
                
                if(strcmp(nickname,"/nickname") == 0){
                    cin >> username;
                    if(strlen(username) > 49 || strlen(username) < 2){
                        cout << "Nome de usuario tem que ser menor que 50 e maio que 2 caracteres" << endl;
                        cout << "Digite novamente /nickname seuNomeUsuario para determinar o seu nome de usuário" << endl;
                        memset(username, 0, strlen(username));
                    }else
                        break;
                }else
                    cout << "\nComando invalido, tente novamente" << endl;
            }
            
            //enviar o nome de usuario para o servidor
            send(client_socket,username,sizeof(username),0);

            thread send_thread(send_message, client_socket);
            thread recv_thread(recv_message, client_socket);

            t_send = move(send_thread);
            t_recv = move(recv_thread);

            if(t_send.joinable())
                t_send.join();
            if(t_recv.joinable())
                t_recv.join();
        }else{
            cout << "Nao foi possivel estabelecer uma conexao\nSe deseja tentar novamente digite 0, se deseja sair digite 1" <<endl;
            cin >> op;
        }   
    }
    	
}









