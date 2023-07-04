#include "channel.h"
#include "../util.h"
#include "user.h"
#include <iostream>
#include <regex>

using namespace std;

Channel::Channel(string name, User *admin) {
  lock_guard<mutex> guard(members_mtx);

  if (!util::is_name_valid(name)) {
    cerr << "Channel name is invalid." << endl;
    exit(EXIT_FAILURE);
  }

  this->name = name;
  this->admin = admin;
  this->members[admin->get_name()] = admin;
}

bool Channel::set_name(string name) {
  if (!util::is_name_valid(name)) {
    return false;
  }

  this->name = name;
  return true;
}

string Channel::get_name() { return name; }

vector<User *> Channel::get_members() {
  std::vector<User *> members_vector;
  members_vector.reserve(members.size());

  for (auto &kv : members) {
    members_vector.push_back(kv.second);
  }

  return members_vector;
}

User *Channel::get_admin() { return admin; }

void Channel::broadcast(string message) {
  auto users = get_members();

  for (auto &user : users) {
    util::send(user->get_socket(), SIG_NOTIF);
    util::send(user->get_socket(), message);
  }
}

bool Channel::add_member(User *user) {
  lock_guard<mutex> guard(members_mtx);

  if (members.size() >= MAX_USERS_PER_CHANNEL ||
      members.find(user->get_name()) != members.end()) {
    return false;
  }

  members[user->get_name()] = user;
  return true;
}

void Channel::remove_member(User *user) {
  lock_guard<mutex> guard(members_mtx);

  if (!members.erase(user->get_name())) {
    return;
  }

  user->unmute();
  broadcast(user->get_name() + " left the channel.");

  // select new admin if needed
  if (user == admin && members.size()) {
    this->admin = members.begin()->second;
    broadcast("New admin is " + this->admin->get_name() + ".");
  }
}

User *Channel::get_member(string name) {
  lock_guard<mutex> guard(members_mtx);

  try {
    return members.at(name);
  } catch (...) {
    return nullptr;
  }
}
