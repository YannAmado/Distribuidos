#include "channel.h"
#include "../util.h"
#include "user.h"
#include <regex>

using namespace std;

Channel::Channel(string name, User *admin) {
  if (!util::is_name_valid(name)) {
    throw "Channel name is invalid.";
  }

  this->name = name;
  this->admin = admin;
  this->members[admin->name] = admin;
}

vector<User *> Channel::get_members() {
  lock_guard<mutex> guard(members_mtx);

  std::vector<User *> members_vector;
  members_vector.reserve(members.size());

  for (auto &kv : members) {
    members_vector.push_back(kv.second);
  }

  return members_vector;
}

User *Channel::get_admin() { return admin; }

void Channel::broadcast(string message) {
  lock_guard<mutex> guard(members_mtx);

  auto users = get_members();
  for (auto &user : users) {
    util::send(user->get_socket(), message);
  }
}

bool Channel::add_member(User *user) {
  lock_guard<mutex> guard(members_mtx);

  if (members.size() >= MAX_USERS_PER_CHANNEL) {
    return false;
  }

  members[user->name] = user;
  return true;
}

void Channel::remove_member(User *user) {
  lock_guard<mutex> guard(members_mtx);
  members.erase(user->name);
  user->unmute();

  // select new admin if needed
  if (user == admin && members.size()) {
    this->admin = members.begin()->second;
  }
}
