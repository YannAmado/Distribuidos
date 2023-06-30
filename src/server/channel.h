#ifndef CHANNEL_H
#define CHANNEL_H

#include "user.h"
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

using namespace std;

class Channel {
  private:
    string name;
    unordered_map<int, User> members;

    static unordered_map<string, Channel> channels;
  public:
    int admin_id;
    Channel(string name, int admin_id);

    vector<User> get_members();
    User *get_admin();
    string get_name();
    void broadcast(string message);

    static Channel *get_by_name(string name);
};

#endif
