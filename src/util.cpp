#include "util.h"
#include "string.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <regex>

void util::send(int socket, string message) {
  // remove invalid characters
  message.erase(remove_if(message.begin(), message.end(),
                          [](char ch) { return !isascii(ch) || iscntrl(ch); }),
                message.end());

  char message_bytes[BYTES_PER_MSG] = {'\0'};
  strcpy(message_bytes, message.c_str());
  ::send(socket, message_bytes, sizeof(message_bytes), 0);
}

string util::recv(int socket) {
  char message[BYTES_PER_MSG];
  ::recv(socket, message, sizeof(message), 0);
  return string(message);
}

string util::get_ip(const sockaddr *addr) {
  char *ip = (char *)malloc(sizeof(char) * 100);

  switch (addr->sa_family) {
  case AF_INET:
    inet_ntop(AF_INET, &(((sockaddr_in *)addr)->sin_addr), ip, 100);
    break;

  case AF_INET6:
    inet_ntop(AF_INET6, &(((sockaddr_in6 *)addr)->sin6_addr), ip, 100);
    break;

  default:
    return "";
  }

  string str(ip);
  free(ip);

  return str;
}

bool util::starts_with(string str, string prefix) {
  return str.rfind(prefix, 0) == 0;
}

bool util::is_name_valid(string name) {
  regex re("^[a-zA-Z][a-zA-Z0-9]+$");
  return name.size() <= MAX_CHAR_NAME && regex_search(name, re);
}

string util::get_cmd_target(string message, string cmd) {
  if (!util::starts_with(message, cmd) || message.size() <= cmd.size() + 1) {
    return "";
  }

  return message.substr(cmd.size() + 1);
}

void util::SignalManager::signalHandler(int signal) { signalHandlerFn(signal); }

void util::SignalManager::init(std::function<void(int)> fn) {
  signalHandlerFn = fn;
  signal(SIGINT, SignalManager::signalHandler);
  signal(SIGQUIT, SignalManager::signalHandler);
}
