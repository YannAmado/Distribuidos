#ifndef USER_H
#define USER_H

#include <string>
#include <thread>
#include <vector>

using namespace std;

class Channel;

class User {
private:
  bool muted;
  int socket;
  string name;

public:
  thread thr;
  Channel *channel;
  string ip;

  User(int socket, string ip);

  void send(string message);
  void mute();
  void unmute();
  bool is_mute();
  int get_socket();
  bool set_name(string name);
  string get_name();

  bool operator==(const User &user);
  bool operator!=(const User &user);
};

#endif
