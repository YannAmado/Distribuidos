#ifndef CHANNEL_H
#define CHANNEL_H

#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

class User;

class Channel {
private:
  User *admin;
  unordered_map<string, User *> members;
  mutex members_mtx;

public:
  string name;

  Channel(string name, User *admin);

  vector<User *> get_members();
  User *get_admin();
  void broadcast(string message);
  /**
   * Add a new member to the channel.
   * @param user the user to be added
   * @return if the user was added (false means either the user is already
   * a member or the channel is full)
   */
  bool add_member(User *user);
  void remove_member(User *user);
};

#endif
