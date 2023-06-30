#include "channel.h"
#include "../util.h"

using namespace std;

vector<User> Channel::get_members() {
  std::vector<User> members_vector;
  members_vector.reserve(members.size());

  for (auto &kv : members) {
    members_vector.push_back(kv.second);
  }

  return members_vector;
}

User *Channel::get_admin() { return &members[admin_id]; }

Channel *Channel::get_by_name(string name) {
  try {
    return &channels.at(name);
  } catch (...) {
    return NULL;
  }
}

Channel::Channel(string name, int admin_id) {
  if (get_by_name(name)) {
    throw "Channel already exists.";
  }

  name = name;
  admin_id = admin_id;
}

void Channel::broadcast(string message) {
  auto users = get_members();
  for (auto &user : users) {
    util::send(user.get_socket(), message);
  }
}
