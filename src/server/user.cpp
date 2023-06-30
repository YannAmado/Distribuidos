#include "../util.h"
#include "channel.h"
#include "user.h"
#include <string>
#include <bits/stdc++.h>

using namespace std;

void User::send(string message) {
  if (is_mute()) {
    return;
  }

  auto users = channel->get_members();
  for (auto &user : users ) {
    if (user != this) {
      util::send(user->get_socket(), message);
    }
  }
}

void User::mute() { this->muted = true; }

void User::unmute() { this->muted = false; }

bool User::is_mute() { return this->muted; }

int User::get_socket() {
  return socket;
}

bool User::operator==(const User &user) { return this->name == user.name; };

bool User::operator!=(const User &user) { return this->name != user.name; };
