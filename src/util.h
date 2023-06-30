#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sys/socket.h>

#define MAX_STR 4096
#define BYTES_PER_MSG 4096
#define MAX_CHANNEL 50
#define INVALID -1

using namespace std;

namespace util {
  void send(int socket, string message);
  string recv(int socket);
  string get_ip(const struct sockaddr *sa);
}

#endif
