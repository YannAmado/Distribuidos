#include "user.h"
#include "../util.h"
#include "channel.h"
#include <bits/stdc++.h>
#include <string>

using namespace std;

User::User(int socket, string ip) {
  if (!util::is_name_valid(name)) {
    throw "User name is invalid.";
  }

  this->socket = socket;
  this->ip = ip;
}

void User::send(string message) {
  if (is_mute()) {
    return;
  }

  auto users = channel->get_members();
  for (auto &user : users) {
    if (user != this) {
      util::send(user->get_socket(), message);
    }
  }
}

void User::mute() { this->muted = true; }

void User::unmute() { this->muted = false; }

bool User::is_mute() { return this->muted; }

int User::get_socket() { return socket; }

bool User::operator==(const User &user) { return this->name == user.name; };

bool User::operator!=(const User &user) { return this->name != user.name; };
