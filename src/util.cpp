#include "util.h"
#include "string.h"
#include <arpa/inet.h>

void util::send(int socket, string message) {
  char message_bytes[BYTES_PER_MSG] = {'\0'};
  strcpy(message_bytes, message.c_str());
  ::send(socket, message_bytes, sizeof(message_bytes), 0);
}

string util::recv(int socket) {
  char message[BYTES_PER_MSG];
  ::recv(socket, message, sizeof(message), 0);
  return string(message);
}

string util::get_ip(const struct sockaddr *sa) {
  char *ip = (char *)malloc(sizeof(char) * 100);

  switch (sa->sa_family) {
    case AF_INET:
      inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), ip, 100);
      break;

    case AF_INET6:
      inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), ip, 100);
      break;

    default:
      strncpy(ip, "Unknown AF", 100);
      return NULL;
  }

  return string(ip);
}
