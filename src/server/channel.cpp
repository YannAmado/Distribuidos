#include "channel.h"
#include "../util.h"
#include "user.h"

using namespace std;

vector<User *> Channel::get_members() {
  std::vector<User *> members_vector;
  members_vector.reserve(members.size());

  for (auto &kv : members) {
    members_vector.push_back(kv.second);
  }

  return members_vector;
}

User *Channel::get_admin() { return admin; }

Channel::Channel(string name, User *admin) {
  name = name;
  admin = admin;
  members[admin->id] = admin;
}

void Channel::broadcast(string message) {
  auto users = get_members();
  for (auto &user : users) {
    util::send(user->get_socket(), message);
  }
}
