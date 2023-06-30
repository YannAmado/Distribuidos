#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sys/socket.h>
#include <vector>

#define MAX_STR 4096
#define BYTES_PER_MSG 4096
#define MAX_USERS_PER_CHANNEL 50
#define INVALID -1
#define KICK_SIG "#KICK"

using namespace std;

namespace util {
  void send(int socket, string message);
  string recv(int socket);
  string get_ip(const struct sockaddr *sa);
  bool starts_with(string str, string prefix);
}

#endif
