#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <errno.h>
#include <mutex>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#define MAX_STR 4096
#define BYTES_PER_MSG 4096
#define MAX_CHANNEL 50

using namespace std;

typedef struct {
  int id;
  string name;
  int socket;
  thread th;
  bool adm;
  string channel;
  bool mute;
  char *ip;
} servidor;

char **canal_nome; // altereção canal
char **mute;
vector<servidor> clients;
int contagem_cliente = 0;
mutex cout_mtx, clients_mtx;

// Sincroniza os terminais
void terminal_compartilhado(string str, bool endLine = true) {
  lock_guard<mutex> guard(cout_mtx);
  cout << str;
  if (endLine)
    cout << endl;
}


bool get_mute(char username[]) {
  for (int i = 0; i < MAX_CHANNEL; i++) {
    if (strcmp(mute[i], username) == 0)
      return true;
  }
  return false;
}

void broadcast_file(char* file_path, int sender_id, char sender_channel[],
                           char name[]){
  int n;
  FILE* fp;
  char buffer[MAX_STR];

  fp = fopen(file_path, "w");
  if(fp == NULL){
    cout << "Erro ao ler o arquivo" << endl;
    return;
  }

  for (int i = 0; i < clients.size(); i++) {
    int helper = (clients[i].channel.compare(sender_channel));

    if (clients[i].id != sender_id && helper == 0 && get_mute(name) != true)
          n = recv(clients[i].socket, buffer, MAX_STR, 0);
          if(n <= 0){
            break;
            return;
          }
          fprintf(fp, "%s", buffer);
          bzero(buffer, MAX_STR);
  }
}

// Manda uma string para todos os usuarios
void broadcast_message_str(string message, int sender_id, char sender_channel[],
                           char name[]) {
  char temp[BYTES_PER_MSG] = {'\0'};
  strcpy(temp, message.c_str());

  for (int i = 0; i < clients.size(); i++) {
    int helper = (clients[i].channel.compare(sender_channel));

    if (clients[i].id != sender_id && helper == 0 && get_mute(name) != true)
      send(clients[i].socket, temp, sizeof(temp), 0);
  }
}

void broadcast_message_str_unico_usuario(string message, int sender_id) {
  char str[BYTES_PER_MSG] = {'\0'};
  strcpy(str, message.c_str());

  for (int i = 0; i < clients.size(); i++) {
    if (clients[i].id == sender_id)
      send(clients[i].socket, str, sizeof(str), 0);
  }
}

// Manda um numero para todos os usuarios (configuração nescessaria para separar
// eles)
void broadcast_message(int num, int sender_id, char sender_channel[],
                       char name[]) {
  for (int i = 0; i < clients.size(); i++) {
    int helper = (clients[i].channel.compare(sender_channel));

    if (clients[i].id != sender_id && helper == 0 && get_mute(name) != true)
      send(clients[i].socket, &num, sizeof(num), 0);
  }
}

void broadcast_message_unico_usuario(int num, int sender_id) {
  for (int i = 0; i < clients.size(); i++) {

    if (clients[i].id == sender_id)
      send(clients[i].socket, &num, sizeof(num), 0);
  }
}


void end_connection(int id) {
  for (int i = 0; i < clients.size(); i++) {
    if (clients[i].id == id) {
      lock_guard<mutex> guard(clients_mtx);
      clients[i].th.detach();
      clients.erase(clients.begin() + i);
      close(clients[i].socket);
      break;
    }
  }
}

bool get_adm(int id) {
  for (int i = 0; i < clients.size(); i++) {
    if (clients[i].id == id)
      return clients[i].adm;
  }
  return 0;
}

// configura o nome do cliente
void set_name(int id, char name[], char channel_name[], bool adm) {
  for (int i = 0; i < clients.size(); i++) {
    if (clients[i].id == id) {
      clients[i].name = string(name);
      clients[i].channel = string(channel_name);
      clients[i].adm = adm;
      cout << "ADM: " << adm << "\n";
    }
  }
}

void inicilializa_canal() {
  canal_nome = (char **)malloc(MAX_CHANNEL * (sizeof(char *)));

  for (int i = 0; i < MAX_CHANNEL; i++)
    canal_nome[i] = (char *)malloc(MAX_STR * (sizeof(char)));

  for (int i = 0; i < MAX_CHANNEL; i++)
    canal_nome[i][0] = '\0';
}

void inicilializa_mute() {
  mute = (char **)malloc(MAX_CHANNEL * (sizeof(char *)));

  for (int i = 0; i < MAX_CHANNEL; i++)
    mute[i] = (char *)malloc(MAX_STR * (sizeof(char)));

  for (int i = 0; i < MAX_CHANNEL; i++)
    mute[i][0] = '\0';
}

void free_mute() {
  for (int i = 0; i < MAX_CHANNEL; i++)
    free(mute[i]);

  free(mute);
}

void free_canal() {
  for (int i = 0; i < MAX_CHANNEL; i++)
    free(canal_nome[i]);

  free(canal_nome);
}

// PRECISA TRATAR A SITUACAO ONDE O CANAL ESTA CHEIO
void configura_canal(int id, char name[], char channel_name[]) {

  bool existe = false;
  for (int i = 0; i < MAX_CHANNEL; ++i) {
    if (strcmp(canal_nome[i], channel_name) == 0)
      existe = true;
  }

  if (existe == true)
    set_name(id, name, channel_name, false);
  else {
    for (int i = 0; i < MAX_CHANNEL; ++i) {
      if (canal_nome[i][0] == '\0') {
        canal_nome[i] = channel_name;
        i = MAX_CHANNEL;
      }
    }
    cout << "Novo canal criado: " << channel_name << endl;
    // se o canal nao existe configura o usuario como adm
    set_name(id, name, channel_name, true);
  }
}

void configura_cliente(int client_socket, int id) {
  char name[MAX_STR], str[BYTES_PER_MSG];
  char channel_name[BYTES_PER_MSG];
  string bem_vindo;

  string id_char;
  id_char = to_string(id);

  // recebe  as mensagens e quem enviou
  recv(client_socket, name, sizeof(name), 0);
  recv(client_socket, channel_name, sizeof(channel_name), 0);
  configura_canal(id, name, channel_name);

  if (true == get_adm(id))
    bem_vindo =
        string("-") + string(name) +
        string(" entrou!\n Este é um novo canal, por pdrão você será o adm!\n");
  else
    bem_vindo = string(name) + string(" entrou!");

  // envia as mensagens para os demais usuarios dentro do canal
  broadcast_message_str("#NULL", id, channel_name, name);
  broadcast_message(id, id, channel_name, name);
  broadcast_message_str(bem_vindo, id, channel_name, name);
  terminal_compartilhado(id_char + bem_vindo, channel_name);

  while (1) {
    memset(str, 0, strlen(str));
    int bytes_received = recv(client_socket, str, sizeof(str), 0);

    if (bytes_received <= 0)
      return;

    // sair do chat
    cout << str << endl;

    if ((strcmp(str, "/quit") == 0) || (int)str[0] == 0) {
      string message = string(name) + string(" - saiu do chat!");

      broadcast_message_str("#NULL", id, channel_name, name);
      broadcast_message(id, id, channel_name, name);
      broadcast_message_str(message, id, channel_name, name);
      terminal_compartilhado(id_char + message);
      end_connection(id);
      return;
    }

    // servidor retorna /pong
    else if (strcmp(str, "/ping") == 0) {
      string message = string("/pong");

      broadcast_message_str_unico_usuario("#NULL", id);
      broadcast_message_unico_usuario(id, id);
      broadcast_message_str_unico_usuario(message, id);
      terminal_compartilhado(id_char + message);
    }

    // expulsar um usuario
    else if (strncmp(str, "/kick", 5) == 0 && get_adm(id)) {
      char user[MAX_STR] = {'\0'};
      for (int i = 6, j = 0; i < strlen(str); i++, j++)
        user[j] = str[i];

      for (int i = 0; i < clients.size(); i++) {
        if (clients[i].name.compare(user) == 0) {
          cout << "Kick " << clients[i].name << endl;
          send(clients[i].socket, "#KICK", sizeof("#KICK"), 0);
        }
      }
    }

    // mutar um usuario
    else if (strncmp(str, "/mute", 5) == 0 && get_adm(id) == true) {
      char user[MAX_STR] = {'\0'};
      for (int i = 6, j = 0; i < strlen(str); i++, j++)
        user[j] = str[i];

      for (int i = 0; i < MAX_CHANNEL; i++) {
        if (mute[i][0] == '\0')
          strcpy(mute[i], user);
      }
    }

    // desmutar um usuario
    else if (strncmp(str, "/unmute", 7) == 0 && get_adm(id) == true) {
      char user[MAX_STR] = {'\0'};
      for (int i = 8, j = 0; i < strlen(str); i++, j++)
        user[j] = str[i];

      for (int i = 0; i < MAX_CHANNEL; i++) {
        if (strcmp(mute[i], user) == 0)
          mute[i][0] = '\0';
      }
    }

    // obter o ip de um usuario
    else if (strncmp(str, "/whois", 6) == 0 && get_adm(id) == true) {
      char user[MAX_STR] = {'\0'};
      for (int i = 7, j = 0; i < strlen(str); i++, j++)
        user[j] = str[i];

      for (int i = 0; i < contagem_cliente; i++) {

        if ((clients[i].name.compare(user)) == 0) {
          cout << "\n\n" << clients[i].ip << "\n\n";
          string str(clients[i].ip);
          string message = "IP de " + string(user) + ": " + str;

          broadcast_message_str_unico_usuario("#NULL", id);
          broadcast_message_unico_usuario(id, id);
          broadcast_message_str_unico_usuario(message, id);
        }
      }
    }

    // para enviar arquivos
    else if (strncmp(str, "/file", 5) == 0){
      char file_message[MAX_STR] = {'\0'};
      for (int i = 6, j = 0; i < strlen(str); i++, j++)
        file_message[j] = str[i];

      // enviando a mensagem que um usuario enviou um arquivo
      string message = string(name) + string(" - Enviou um arquivo! Digite /salvar para salvar o arquivo");
      broadcast_message_str("#NULL", id, channel_name, name);
      broadcast_message(id, id, channel_name, name);
      broadcast_message_str(message, id, channel_name, name);

      // enviando o conteudo do arquivo
      broadcast_message_str("#FILE", id, channel_name, name);
      broadcast_message(id, id, channel_name, name);
      broadcast_message_str(file_message, id, channel_name, name);
      terminal_compartilhado(id_char + message);

    }
    else if (strncmp(str, "/salvar", 7) == 0){

    }
    // para enviar mensagens normalmente
    else {
      broadcast_message_str(string(name), id, channel_name, name);
      broadcast_message(id, id, channel_name, name);
      broadcast_message_str(string(str), id, channel_name, name);
      string message =
          string(id_char) + string("-") + string(name) + string(":");
      terminal_compartilhado(message + str);
    }
  }
}

char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen) {
  switch (sa->sa_family) {
  case AF_INET:
    inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen);
    break;

  case AF_INET6:
    inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
    break;

  default:
    strncpy(s, "Unknown AF", maxlen);
    return NULL;
  }
  return s;
}

int main() {

  struct sockaddr_in client;
  int client_socket;
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (server_socket == -1) {
    cout << "Erro: server_socket, reinicie o servidor" << endl;
    return EXIT_FAILURE;
  } 

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(9002);
  server.sin_addr.s_addr = INADDR_ANY;
  bzero(&server.sin_zero, 0);

  unsigned int len = sizeof(sockaddr_in);

  if ((bind(server_socket, (struct sockaddr *)&server,
            sizeof(struct sockaddr_in))) == -1) {
    perror("bind error: ");
    exit(0);
  }

  if ((listen(server_socket, 8)) == -1) {
    perror("listen error: ");
    exit(0);
  }

  inicilializa_canal();
  inicilializa_mute();

  while (1) {
    client_socket = accept(server_socket, (struct sockaddr *)&client, &len);
    char *s = (char *)malloc(sizeof(char) * 100);
    char *client_ip = get_ip_str((struct sockaddr *)&client, s, 100);

    if (client_socket == -1) {
      printf("Erro ao se conectar\n\n");
      exit(0);
    } else
      printf("Conectado ao cliente\n");

    contagem_cliente++;
    thread t(configura_cliente, client_socket, contagem_cliente);
    lock_guard<mutex> guard(clients_mtx);
    clients.push_back(
        {contagem_cliente, string("Anonymous"), client_socket, (move(t))});
    clients[contagem_cliente - 1].ip = client_ip;
  }

  for (int i = 0; i < clients.size(); i++) {
    if (clients[i].th.joinable())
      clients[i].th.join();
  }

  free_mute();
  free_canal();
  close(server_socket);
  return 0;
}
