#include "user.h"
#include "../util.h"
#include "channel.h"
#include <bits/stdc++.h>
#include <string>

using namespace std;

User::User(int socket, string ip) {
  this->muted = false;
  this->socket = socket;
  this->ip = ip;
  this->name = "";
  this->channel = nullptr;
}

bool User::set_name(string name) {
  if (!util::is_name_valid(name)) {
    this->name = "";
    return false;
  }

  this->name = name;
  return true;
}

string User::get_name() {
  return name;
}

void User::send(string message) {
  if (is_mute()) {
    return;
  }

  auto users = channel->get_members();
  for (auto &user : users) {
    if (user != this) {
      util::send(user->get_socket(), SIG_MESSSAGE);
      util::send(user->get_socket(), this->get_name());
      util::send(user->get_socket(), message);
    }
  }
}

void User::mute() { muted = true; }

void User::unmute() { muted = false; }

bool User::is_mute() { return muted; }

int User::get_socket() { return socket; }

bool User::operator==(const User &user) { return name == user.name; };

bool User::operator!=(const User &user) { return name != user.name; };
