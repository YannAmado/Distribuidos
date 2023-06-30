#include "../util.h"
#include "channel.h"
#include <string>
#include <bits/stdc++.h>

using namespace std;

void User::send(string message) {
  if (is_mute()) {
    return;
  }

  auto users = Channel::get_by_name(channel)->get_members();
  for (auto &user : users ) {
    if (user.id != id) {
      util::send(user.get_socket(), message);
    }
  }
}

void User::mute() { this->muted = true; }

void User::unmute() { this->muted = false; }

bool User::is_mute() { return this->muted; }

int User::get_socket() {
  return socket;
}
