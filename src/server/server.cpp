#include "server.h"
#include "../util.h"
#include "channel.h"
#include "user.h"

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

unordered_map<string, Channel> channels;
unordered_map<string, User> users;
mutex log_mtx, clients_mtx;

void Server::log(string message) {
  lock_guard<mutex> guard(log_mtx);
  cout << message << endl;
}

void Server::dm(User *target, string message) {
  util::send(target->get_socket(), message);
}

void Server::disconnect(User *user) {
  lock_guard<mutex> guard(clients_mtx);
  user->thr.detach();
  close(user->get_socket());
  users.erase(user->name);
  free(user);
}

User *get_cmd_target(string message, string cmd) {
  if (!util::starts_with(message, cmd) || message.size() <= cmd.size() + 1) {
    return NULL;
  }

  string target_name = message.substr(cmd.size() + 1);

  try {
    return &users.at(target_name);
  } catch(...) {
    return NULL;
  }
}

void Server::listen_client(User *user) {
  int client_socket = user->get_socket();

  // get user and channel name
  user->name = util::recv(client_socket);
  string channel_name = util::recv(client_socket);

  // create or get channel
  Channel *channel;
  try {
    channel = &channels.at(channel_name);
  } catch (...) {
    channels[channel_name] = Channel(channel_name, user);
    channel = &channels.at(channel_name);
  }

  // greet user
  string welcome_msg =
      channel->get_admin() == user
          ? "[You just created the channel '" + channel_name + "' and is the admin.]"
          : "[" + user->name + "just entered the channel.]";
  channel->broadcast(welcome_msg);

  while (1) {
    string message = util::recv(client_socket);

    if (message == QUIT_CMD || message == "") {
      /* broadcast_message_str("#NULL", user.id, channel_name, name); */
      /* broadcast_message(user.id, user.id, channel_name, name); */
      /* broadcast_message_str(message, user.id, channel_name, name); */
      /* terminal_compartilhado(id_char + message); */

      channel->broadcast("[" + user->name + "left the channel.]");
      disconnect(user);
      return;
    } else if (message == PING_CMD) {
      channel->broadcast("pong");
      continue;
    } else if (user == channel->get_admin()) {
      if (util::starts_with(message, KICK_CMD)) {
        User *target = get_cmd_target(message, KICK_CMD);

        if (target == NULL) {
          dm(user, (string)"[ERROR] Target not found for command " + KICK_CMD);
          // TODO: log
        }

        util::send(target->get_socket(), KICK_SIG);
        continue;
      } else if (message == MUTE_CMD) {
        User *target = get_cmd_target(message, MUTE_CMD);

        if (target == NULL) {
          dm(user, (string)"[ERROR] Target not found for command " + MUTE_CMD);
          // TODO: log
        }

        target->mute();
        continue;
      } else if (message == UNMUTE_CMD) {
        User *target = get_cmd_target(message, UNMUTE_CMD);

        if (target == NULL) {
          dm(user, (string)"[ERROR] Target not found for command " + UNMUTE_CMD);
          // TODO: log
        }

        target->unmute();
        continue;
      } else if (message == WHOIS_CMD) {
        User *target = get_cmd_target(message, MUTE_CMD);

        if (target == NULL) {
          dm(user, (string)"[ERROR] Target not found for command " + MUTE_CMD);
          // TODO: log
        }

        dm(target, "-> " + target->name + "'s IP is: " + target->ip);
        continue;
      }
    }

    user->send(message);
  }
}

void Server::launch() {
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
    exit(EXIT_FAILURE);
  }

  if (bind(server_socket, (sockaddr *)&server_sockaddr, sizeof(sockaddr_in)) ==
      INVALID) {
    perror("bind error: ");
    exit(EXIT_FAILURE);
  }

  if ((listen(server_socket, 8)) == INVALID) {
    perror("listen error: ");
    exit(EXIT_FAILURE);
  }

  while (true) {
    client_socket = accept(server_socket, (struct sockaddr *)&client_sockaddr,
        &sizeof(struct sockaddr_in));

    if (client_socket == INVALID) {
      cout << "[ERROR] Couldn't connect to client" << endl;
      exit(0);
    } else {
      cout << "[INFO] Successfully connected to client" << endl;
    }

    User new_user(client_socket,
        util::get_ip((struct sockaddr *)&client_sockaddr));

    thread thr(&Server::listen_client, this, new_user);
    // TODO: move to inside user.cpp
    lock_guard<mutex> guard(clients_mtx);

    new_user.thr = ::move(thr);
  }

  for (int i = 0; i < clients.size(); i++) {
    if (clients[i].th.joinable())
      clients[i].th.join();
  }

  close(server_socket);
}
