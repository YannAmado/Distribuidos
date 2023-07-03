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
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

using namespace std;

unordered_map<string, Channel*> channels;
unordered_map<string, User*> users;
mutex log_mtx, users_mtx, channels_mtx;

void Server::log(string message) {
  lock_guard<mutex> guard(log_mtx);
  cout << message << endl;
}

void Server::dm(User *target, string message) {
  util::send(target->get_socket(), message);
}

void Server::disconnect(User *user) {
  lock_guard<mutex> guard(users_mtx);
  user->thr.detach();
  close(user->get_socket());
  user->channel->remove_member(user);
  users.erase(user->name);
  free(user);
}

void Server::close_channel(Channel *channel) {
  lock_guard<mutex> guard(channels_mtx);
  channels.erase(channel->name);
  free(channel);
}

string get_cmd_target(string message, string cmd) {
  if (!util::starts_with(message, cmd) || message.size() <= cmd.size() + 1) {
    return "";
  }

  return message.substr(cmd.size() + 1);
}

User *user_cmd_target(string message, string cmd) {
  try {
    return users.at(get_cmd_target(message, cmd));
  } catch (...) {
    return NULL;
  }
}

void Server::listen_client(User *user) {
  int client_socket = user->get_socket();

  // get username that's not in use
  do {
    user->name = util::recv(client_socket);
    users_mtx.lock();

    if (!util::is_name_valid(user->name)) {
      dm(user, ERROR_SIG);
      dm(user, "[ERROR] This nickname is invalid, it must start with a "
                "letter, contain only letters and numbers and have at most " +
                    to_string(MAX_CHAR_NAME) + " characters.");
      log("[INFO] New user tried choosing username '" + user->name +
          "', which is invalid.");

      user->name = "";
    } else if (users.find(user->name) != users.end()) {
      dm(user, ERROR_SIG);
      dm(user,
         "[ERROR] This nickname is already in use, try choosing another.");
      log("[INFO] New user tried choosing username '" + user->name +
          "', which is already in use.");

      user->name = "";
    }

    users_mtx.unlock();
  } while (user->name == "");

  // save user
  users_mtx.lock();
  users.emplace(user->name, user);
  users_mtx.unlock();

  while (1) {
    string message = util::recv(client_socket);

    if (!user->channel) {
      string channel_name = get_cmd_target(message, JOIN_CMD);
      if (channel_name == "") {
        dm(user, ERROR_SIG);
        continue;
      } else if (!util::is_name_valid(channel_name)) {
        dm(user, ERROR_SIG);
        dm(user, "[ERROR] This channel name is invalid, it must start with a "
                  "letter, contain only letters and numbers and have at most " +
                      to_string(MAX_CHAR_NAME) + " characters.");
        log("[INFO] User '" + user->name + "' tried joining channel '" +
            channel_name + "', which is invalid.");
        continue;
      }

      // create or get channel
      channels_mtx.lock();
      try {
        user->channel = channels.at(channel_name);
      } catch (...) {
        user->channel = (Channel *)malloc(sizeof(Channel ));
        new(user->channel) Channel(channel_name, user);
      }
      channels_mtx.unlock();

      // reject connection if channel is full
      if (user->channel->get_members().size() >= MAX_USERS_PER_CHANNEL) {
        dm(user, ERROR_SIG);
        dm(user, "[ERROR] The channel '" + channel_name +
                      "' is full, try joining another.");
        log("[INFO] User '" + user->name + "' tried joining channel '" +
            channel_name + "', which is full with " +
            to_string(user->channel->get_members().size()) + "members.");

        user->channel = NULL;
      }

      // add member to channel
      user->channel->add_member(user);
      dm(user, SUCCESS_SIG);

      // greet user
      string welcome_msg = user->channel->get_admin() == user
                               ? "[You just created the channel '" +
                                     user->channel->name +
                                     "' and is the admin.]"
                               : "[" + user->name + "just entered the channel.]";
      user->channel->broadcast(welcome_msg);
    } else if (message == QUIT_CMD || message == "") {
      Channel *channel = user->channel;

      channel->broadcast("['" + user->name + "' left the channel.]");
      log("[INFO] '" + user->name + "' left the channel '" +
          channel->name + "'.");
      disconnect(user);

      if (channel->get_members().size() == 0) {
        close_channel(channel);
        log("[INFO] Channel '" + user->channel->name +
            "' was deleted because all members left.");
      }

      return;
    } else if (message == PING_CMD) {
      dm(user, "-> pong");
      continue;
    } else if (user == user->channel->get_admin()) {
      if (util::starts_with(message, KICK_CMD)) {
        User *target = user_cmd_target(message, KICK_CMD);

        if (target == NULL) {
          dm(user,
             (string) "[ERROR] Target not found for command " + KICK_CMD);
          log((string) "[INFO] Target not found for command " + KICK_CMD);
        } else {
          util::send(target->get_socket(), KICK_SIG);
          log("[INFO] User " + target->name + " was kicked from channel " +
              target->channel->name + ".");
          target->channel->remove_member(target);
        }

        continue;
      } else if (message == MUTE_CMD) {
        User *target = user_cmd_target(message, MUTE_CMD);

        if (target == NULL) {
          dm(user,
             (string) "[ERROR] Target not found for command " + MUTE_CMD);
          log((string) "[INFO] Target not found for command " + MUTE_CMD);
        } else {
          target->mute();
          log("[INFO] User " + target->name + " was muted by user " +
              user->name + ".");
        }

        continue;
      } else if (message == UNMUTE_CMD) {
        User *target = user_cmd_target(message, UNMUTE_CMD);

        if (target == NULL) {
          dm(user,
             (string) "[ERROR] Target not found for command " + UNMUTE_CMD);
          log((string) "[INFO] Target not found for command " + UNMUTE_CMD);
        } else {
          target->unmute();
          log("[INFO] User " + target->name + " was unmuted by user " +
              user->name + ".");
        }

        continue;
      } else if (message == WHOIS_CMD) {
        User *target = user_cmd_target(message, WHOIS_CMD);

        if (target == NULL) {
          dm(user,
             (string) "[ERROR] Target not found for command " + WHOIS_CMD);
          log((string) "[INFO] Target not found for command " + WHOIS_CMD);
        } else {
          dm(target, "-> " + target->name + "'s IP is: " + target->ip);
          log("[INFO] User " + user->name + " read " + target->name + "'s IP (" +
              target->ip + ").");
        }

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
  /* bzero(&server_sockaddr.sin_zero, 0); */

  int server_socket = socket(server_sockaddr.sin_family, SOCK_STREAM, 0);
  unsigned int socklen = sizeof(struct sockaddr_in);

  if (server_socket == INVALID) {
    log("[ERROR] Couldn't define server_socket, reset server");
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

  log("[INFO] Running...");

  while (true) {
    log("[INFO] Waiting for client connection...");

    client_socket =
        accept(server_socket, (struct sockaddr *)&client_sockaddr, &socklen);

    if (client_socket == INVALID) {
      log("[ERROR] Couldn't connect to client");
      continue;
    } else {
      log("[INFO] Successfully connected to client");
    }

    User *new_user = (User *)malloc(sizeof(User));
    new (new_user)
        User(client_socket, util::get_ip((struct sockaddr *)&client_sockaddr));

    thread thr(&Server::listen_client, this, new_user);
    new_user->thr = ::move(thr);
  }

  for (auto &kv : users) {
    User *user = kv.second;

    if (user->thr.joinable()) {
      user->thr.join();
    }

    dm(user, TERMINATE_SIG);
    close(user->get_socket());
    free(user);
  }

  for (auto &kv : channels) {
    free(kv.second);
  }

  close(server_socket);
}
