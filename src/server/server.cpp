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

unordered_map<string, Channel *> channels;
unordered_map<string, User *> users;
vector<User *> connecting_users;
mutex log_mtx, users_mtx, channels_mtx;

void Server::log(string message) {
  lock_guard<mutex> guard(log_mtx);
  cout << "[INFO] " << message << endl;
}

void Server::err(string message) {
  lock_guard<mutex> guard(log_mtx);
  cout << "[ERROR] " << message << endl;
}

void Server::send(User *target, string message) {
  util::send(target->get_socket(), message);
}

void Server::disconnect(User *user) {
  lock_guard<mutex> guard(users_mtx);
  users.erase(user->get_name());

  if (user->channel) {
    user->channel->remove_member(user);
    user->channel = nullptr;
  }

  user->thr.detach();
  close(user->get_socket());
  free(user);
}

void Server::close_channel(Channel *channel) {
  lock_guard<mutex> guard(channels_mtx);
  channels.erase(channel->get_name());
  free(channel);
}

/** @return user that is a target of `cmd` in `message` as long as they are in
 * `channel` */
User *user_cmd_target(string message, string cmd, Channel *channel) {
  return channel->get_member(util::get_cmd_target(message, cmd));
}

void Server::listen_client_handler(User *user) {
  while (true) {
    string message = util::recv(user->get_socket());
    string username_safe = user->get_name() == "" ? "new user" : user->get_name();

    log("Received message from " + username_safe);

    if (message == QUIT_CMD) {
      Channel *channel = user->channel;

      if (channel) {
        log(user->get_name() + " is leaving the channel '" +
            channel->get_name() + "'.");

        channel->remove_member(user);
        user->channel = nullptr;

        if (channel->get_members().size() == 0) {
          close_channel(channel);
          log("Channel " + channel->get_name() +
              " was deleted because all members left.");
        }
      }

      log("Disconnecting " + username_safe + "...");

      send(user, SIG_SUCCESS);
      send(user, QUIT_CMD);
      send(user, "Quitting...");
      disconnect(user);

      log("Disconnected " + username_safe);

      return;
    } else if (user->get_name() == "") {
      string nickname = util::get_cmd_target(message, NICKNAME_CMD);
      bool is_name_valid = user->set_name(nickname);
      lock_guard<mutex> guard(users_mtx);

      if (!util::starts_with(message, NICKNAME_CMD)) {
        send(user, SIG_ERROR);
        send(user, (string) "You don't have a nickname yet. Use the command '" +
                       NICKNAME_CMD + " yourNickname' to set one.");
        log("New user tried sendind '" + message +
            "', but they don't have a nickname yet.");

        continue;
      } else if (!is_name_valid) {
        send(user, SIG_ERROR);
        send(user,
             "This nickname is invalid, it must start with a "
             "letter, contain only letters and numbers and have at most " +
                 to_string(MAX_CHAR_NAME) + " characters.");
        log("New user tried choosing username '" + nickname +
            "', which is invalid.");

        user->set_name("");
        continue;
      } else if (users.find(user->get_name()) != users.end()) {
        send(user, SIG_ERROR);
        send(user, "This nickname is already in use, try choosing another.");
        log("New user tried choosing username '" + user->get_name() +
            "', which is already in use.");

        user->set_name("");
        continue;
      }

      // save user
      send(user, SIG_SUCCESS);
      send(user, NICKNAME_CMD);
      send(user, (string) "You set your nickname to '" + user->get_name() +
                     "'. Now use the command '" + JOIN_CMD +
                     " channelName' to join a channel.");

      connecting_users.erase(
          remove(connecting_users.begin(), connecting_users.end(), user),
          connecting_users.end());
      users.emplace(user->get_name(), user);

      log("User " + user->get_name() + " connected. " +
          to_string(connecting_users.size()) + " users still connecting.");
      continue;
    } else if (!user->channel) {
      string channel_name = util::get_cmd_target(message, JOIN_CMD);

      if (!util::starts_with(message, JOIN_CMD)) {
        send(user, SIG_ERROR);
        send(user, (string) "You're not in a channel. Use the command '" +
                       JOIN_CMD + " channelName' to join a channel.");
        log("User '" + user->get_name() + "' tried sending '" + message +
            "', but they aren't in a channel yet.");
        continue;
      } else if (!util::is_name_valid(channel_name)) {
        send(user, SIG_ERROR);
        send(user,
             "This channel name is invalid, it must start with a "
             "letter, contain only letters and numbers and have at most " +
                 to_string(MAX_CHAR_NAME) + " characters.");
        log("User '" + user->get_name() + "' tried joining channel '" +
            channel_name + "', which is invalid.");
        continue;
      }

      log(user->get_name() + " is attempting to join channel " + channel_name);

      // create or get channel
      channels_mtx.lock();
      try {
        user->channel = channels.at(channel_name);

        log(channel_name + " already exists with " +
            to_string(user->channel->get_members().size()) +
            " members. Attempting to add " + user->get_name());
      } catch (...) {
        log(channel_name + " doesn't exist. Attempting to create it for " +
            user->get_name());

        user->channel = (Channel *)malloc(sizeof(Channel));
        new (user->channel) Channel(channel_name, user);
        channels[channel_name] = user->channel;

        log("Channel " + channel_name + " created.");
      }
      channels_mtx.unlock();

      // reject if channel is full
      if (user->channel->get_members().size() >= MAX_USERS_PER_CHANNEL) {
        send(user, SIG_ERROR);
        send(user, "The channel '" + channel_name +
                       "' is full, try joining another.");
        log("User '" + user->get_name() + "' tried joining channel '" +
            channel_name + "', which is full with " +
            to_string(user->channel->get_members().size()) + " members.");

        user->channel = nullptr;
        continue;
      }

      // notify members
      string greetings;
      if (user != user->channel->get_admin()) {
        user->channel->broadcast(user->get_name() +
                                 " just joined the channel.");
        greetings = "You successfully joined the channel " +
                    user->channel->get_name() + ".";
      } else {
        greetings = "You just created the channel '" +
                    user->channel->get_name() + "' and is now the admin.";
      }

      log(user->get_name() + greetings.substr(3));

      // add member to channel
      user->channel->add_member(user);
      send(user, SIG_SUCCESS);
      send(user, JOIN_CMD);
      send(user, greetings);

      continue;
    } else if (message == PING_CMD) {
      log(user->get_name() + " just pinged.");
      send(user, SIG_SUCCESS);
      send(user, PING_CMD);
      send(user, "pong");
      log("Ponged back to " + user->get_name());
      continue;
    } else if (user == user->channel->get_admin()) {
      if (util::starts_with(message, KICK_CMD)) {
        User *target = user_cmd_target(message, KICK_CMD, user->channel);

        if (!target) {
          send(user, SIG_ERROR);
          send(user, (string) "Target not found for command " + KICK_CMD);
          log((string) "Target not found for command " + KICK_CMD);
          continue;
        } else if (user == target) {
          send(user, SIG_ERROR);
          send(user, "You can't kick yourself.");
          log("User " + user->get_name() + " tried kicking themselves.");
          continue;
        }

        log(user->get_name() + " tried to kick " + target->get_name() +
            " from channel " + user->channel->get_name());
        send(target, SIG_KICK);
        send(target,
             "You were kicked from channel " + user->channel->get_name() + ".");

        target->channel = nullptr;
        user->channel->remove_member(target);

        log("User " + target->get_name() + " was kicked from channel " +
            user->channel->get_name() + ".");

        send(user, SIG_SUCCESS);
        send(user, KICK_CMD);
        send(user, "User " + target->get_name() + " was kicked.");

        continue;
      } else if (util::starts_with(message, MUTE_CMD)) {
        User *target = user_cmd_target(message, MUTE_CMD, user->channel);

        if (!target) {
          send(user, SIG_ERROR);
          send(user, (string) "Target not found for command " + MUTE_CMD);
          log((string) "Target not found for command " + MUTE_CMD);
          continue;
        } else if (user == target) {
          send(user, SIG_ERROR);
          send(user, "You can't mute yourself.");
          log("User " + user->get_name() + " tried muting themselves.");
          continue;
        }

        log(user->get_name() + " tried to mute " + target->get_name() +
            " in channel " + user->channel->get_name());

        target->mute();
        send(target, SIG_NOTIF);
        send(target, "You were just muted.");

        log("User " + target->get_name() + " was muted by user " +
            user->get_name() + ".");

        send(user, SIG_SUCCESS);
        send(user, MUTE_CMD);
        send(user, "You just muted " + target->get_name() + ".");

        continue;
      } else if (util::starts_with(message, UNMUTE_CMD)) {
        User *target = user_cmd_target(message, UNMUTE_CMD, user->channel);

        if (!target) {
          send(user, SIG_ERROR);
          send(user, (string) "Target not found for command " + UNMUTE_CMD);
          log((string) "Target not found for command " + UNMUTE_CMD);
          continue;
        } else if (user == target) {
          send(user, SIG_ERROR);
          send(user, "You can't mute/unmute yourself.");
          log("User " + user->get_name() + " tried unmuting themselves.");
          continue;
        }

        log(user->get_name() + " tried to unmute " + target->get_name() +
            " in channel " + user->channel->get_name());

        send(target, SIG_NOTIF);
        send(target, "You were just unmuted.");

        log("User " + target->get_name() + " was unmuted by user " +
            user->get_name() + ".");

        send(user, SIG_SUCCESS);
        send(user, UNMUTE_CMD);
        send(user, "You just unmuted " + target->get_name() + ".");

        target->unmute();

        continue;
      } else if (util::starts_with(message, WHOIS_CMD)) {
        User *target = user_cmd_target(message, WHOIS_CMD, user->channel);

        if (!target) {
          send(user, SIG_ERROR);
          send(user, (string) "Target not found for command " + WHOIS_CMD);
          log((string) "Target not found for command " + WHOIS_CMD);
          continue;
        }

        log("User " + user->get_name() + " read " + target->get_name() +
            "'s IP: " + target->ip);

        send(user, SIG_SUCCESS);
        send(user, WHOIS_CMD);
        send(user, target->get_name() + "'s IP is: " + target->ip);

        continue;
      }
    }

    user->send(message);
    log(user->get_name() + " sent a message with " + to_string(message.size()) +
        " bytes in " + user->channel->get_name() +
        (user->is_mute() ? " (but is mute)" : ""));
  }
}

void Server::launch() {
  sockaddr_in client_sockaddr;
  int client_socket;

  sockaddr_in server_sockaddr;
  server_sockaddr.sin_family = AF_INET;
  server_sockaddr.sin_port = htons(9002);
  server_sockaddr.sin_addr.s_addr = INADDR_ANY;

  unsigned int socklen = sizeof(sockaddr_in);

  socket = ::socket(server_sockaddr.sin_family, SOCK_STREAM, 0);
  if (socket == INVALID) {
    err("Couldn't define server_socket, reset server");
    exit(EXIT_FAILURE);
  }

  int socket_bind =
      bind(socket, (sockaddr *)&server_sockaddr, sizeof(sockaddr_in));
  if (socket_bind == INVALID) {
    perror("bind error: ");
    exit(EXIT_FAILURE);
  }

  if (listen(socket, 8) == INVALID) {
    perror("listen error: ");
    exit(EXIT_FAILURE);
  }

  log("Running...");

  while (true) {
    log("Waiting for client connection...");

    client_socket = accept(socket, (sockaddr *)&client_sockaddr, &socklen);

    if (client_socket == INVALID) {
      err("Couldn't connect to client");
      continue;
    } else {
      log("Successfully connected to client");
    }

    User *new_user = (User *)malloc(sizeof(User));
    string new_user_ip = util::get_ip((sockaddr *)&client_sockaddr);

    if (new_user_ip == "") {
      util::send(client_socket, SIG_ERROR);
      err("Failed to fetch client IP");
      continue;
    }

    util::send(client_socket, SIG_SUCCESS);
    util::send(client_socket,
               (string) "Connection stablished. Use the command '" +
                   NICKNAME_CMD + " yourNickname' to set a nickname.");

    new (new_user) User(client_socket, new_user_ip);

    users_mtx.lock();
    connecting_users.push_back(new_user);
    users_mtx.unlock();

    thread thr(&Server::listen_client_handler, this, new_user);
    new_user->thr = ::move(thr);
  }

  for (auto &kv : users) {
    if (kv.second->thr.joinable()) {
      kv.second->thr.join();
    }
  }

  teardown();
}

void Server::teardown() {
  log("Tearing down...");

  for (auto &kv : users) {
    User *user = kv.second;

    user->thr.detach();
    send(user, SIG_TERMINATE);
    close(user->get_socket());
    free(user);
  }

  for (auto &user : connecting_users) {
    user->thr.detach();
    send(user, SIG_TERMINATE);
    close(user->get_socket());
    free(user);
  }

  for (auto &kv : channels) {
    free(kv.second);
  }

  close(socket);
  exit(EXIT_SUCCESS);
}
