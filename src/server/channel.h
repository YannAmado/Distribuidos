#ifndef CHANNEL_H
#define CHANNEL_H

#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

using namespace std;

class User;

class Channel {
  private:
    string name;
    User *admin;
    unordered_map<int, User *> members;
  public:
    Channel(string name, User *admin);

    vector<User *> get_members();
    User *get_admin();
    string get_name();
    void broadcast(string message);
};

#endif
