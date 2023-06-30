#include "../util.h"
#include "server.h"
#include "user.h"
#include "channel.h"

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
} client_;

char **canal_nome; // altereção canal
char **mute;
vector<client_> clients;
int contagem_cliente = 0;
mutex cout_mtx, clients_mtx;

// Sincroniza os terminais
void terminal_compartilhado(string str, bool endLine = true) {
  lock_guard<mutex> guard(cout_mtx);
  cout << str;
  if (endLine)
    cout << endl;
}

void Server::dm(User target, string message) {
  util::send(target.get_socket(), message);
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

void client_thread(User user) {
  int client_socket = user.get_socket();

  string bem_vindo;

  string id_char;
  id_char = to_string(user.id);

  user.name = util::recv(client_socket);
  string channel_name = util::recv(client_socket);

  Channel *channel = Channel::get_by_name(channel_name);

  if (channel == NULL) {
    Channel(channel_name, user.id);
    channel = Channel::get_by_name(channel_name);
  }

  string welcome_msg = channel->admin_id == user.id
    ?  "[You just created the " + channel_name + " and is the admin.]"
    : "[" + user.name + "entered the channel.]";
  channel->broadcast(welcome_msg);

  while (1) {
    memset(str, 0, strlen(str));
    int bytes_received = recv(client_socket, str, sizeof(str), 0);

    if (bytes_received <= 0)
      return;

    // sair do chat
    if ((strcmp(str, "/quit") == 0) || (int)str[0] == 0) {
      string message = string(name) + string(" - saiu do chat!");

      broadcast_message_str("#NULL", user.id, channel_name, name);
      broadcast_message(user.id, user.id, channel_name, name);
      broadcast_message_str(message, user.id, channel_name, name);
      terminal_compartilhado(id_char + message);
      end_connection(user.id);
      return;
    }

    // servidor retorna /pong
    else if (strcmp(str, "/ping") == 0) {
      string message = string("/pong");

      broadcast_message_str_unico_usuario("#NULL", user.id);
      broadcast_message_unico_usuario(user.id, user.id);
      broadcast_message_str_unico_usuario(message, user.id);
      terminal_compartilhado(id_char + message);
    }

    // expulsar um usuario
    else if (strncmp(str, "/kick", 5) == 0 && get_adm(user.id)) {
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
    else if (strncmp(str, "/mute", 5) == 0 && get_adm(user.id) == true) {
      char user[MAX_STR] = {'\0'};
      for (int i = 6, j = 0; i < strlen(str); i++, j++)
        user[j] = str[i];

      for (int i = 0; i < MAX_CHANNEL; i++) {
        if (mute[i][0] == '\0')
          strcpy(mute[i], user);
      }
    }

    // desmutar um usuario
    else if (strncmp(str, "/unmute", 7) == 0 && get_adm(user.id) == true) {
      char user[MAX_STR] = {'\0'};
      for (int i = 8, j = 0; i < strlen(str); i++, j++)
        user[j] = str[i];

      for (int i = 0; i < MAX_CHANNEL; i++) {
        if (strcmp(mute[i], user) == 0)
          mute[i][0] = '\0';
      }
    }

    // obter o ip de um usuario
    else if (strncmp(str, "/whois", 6) == 0 && get_adm(user.id) == true) {
      char user[MAX_STR] = {'\0'};
      for (int i = 7, j = 0; i < strlen(str); i++, j++)
        user[j] = str[i];

      for (int i = 0; i < contagem_cliente; i++) {

        if ((clients[i].name.compare(user)) == 0) {
          cout << "\n\n" << clients[i].ip << "\n\n";
          string str(clients[i].ip);
          string message = "IP de " + string(user) + ": " + str;

          broadcast_message_str_unico_usuario("#NULL", user.id);
          broadcast_message_unico_usuario(user.id, user.id);
          broadcast_message_str_unico_usuario(message, user.id);
        }
      }
    }

    // para enviar mensagens normalmente
    else {
      broadcast_message_str(string(name), user.id, channel_name, name);
      broadcast_message(user.id, user.id, channel_name, name);
      broadcast_message_str(string(str), user.id, channel_name, name);
      string message =
          string(id_char) + string("-") + string(name) + string(":");
      terminal_compartilhado(message + str);
    }
  }
}

int main() {
  sockaddr_in client_sockaddr;
  int client_socket;

  sockaddr_in server_sockaddr;
  server_sockaddr.sin_family = AF_INET;
  server_sockaddr.sin_port = htons(9002);
  server_sockaddr.sin_addr.s_addr = INADDR_ANY;
  bzero(&server_sockaddr.sin_zero, 0);
  int server_socket = socket(server_sockaddr.sin_family, SOCK_STREAM, 0);

  if (server_socket == INVALID) {
    cout << "[ERROR] Couldn't define server_socket, reset server" << endl;
    return EXIT_FAILURE;
  }

  if (bind(server_socket, (sockaddr *)&server_sockaddr, sizeof(sockaddr_in)) == INVALID) {
    perror("bind error: ");
    exit(0);
  }

  if ((listen(server_socket, 8)) == INVALID) {
    perror("listen error: ");
    exit(0);
  }

  while (true) {
    client_socket = accept(server_socket, (struct sockaddr *)&client_sockaddr, &sizeof(sockaddr_in));

    if (client_socket == INVALID) {
      cout << "[ERROR] Couldn't connect to client" << endl;
      exit(0);
    } else {
      cout << "[INFO] Successfully connected to client" << endl;
    }

    User new_user(client_socket, util::get_ip((struct sockaddr *)&client_sockaddr));

    thread thr(client_thread, new_user);
    // TODO: move to inside user.cpp
    lock_guard<mutex> guard(clients_mtx);

    new_user.thr = move(thr);
  }

  for (int i = 0; i < clients.size(); i++) {
    if (clients[i].th.joinable())
      clients[i].th.join();
  }

  close(server_socket);

  return 0;
}
