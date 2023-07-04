#include "util.h"
#include "string.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <cstdlib>
#include <regex>
#include <stdexcept>
#include <system_error>

using namespace std;

void util::send_without_filtering(int socket, string message) {
  char message_bytes[BYTES_PER_MSG] = {'\0'};
  strcpy(message_bytes, message.c_str());
  ::send(socket, message_bytes, sizeof(message_bytes), 0);
}

void util::send(int socket, string message) {
  // remove invalid characters
  message.erase(remove_if(message.begin(), message.end(),
                          [](char ch) { return !isascii(ch) || iscntrl(ch); }),
                message.end());
  send_without_filtering(socket, message);
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

void util::SignalManager::init(function<void(int)> fn) {
  signalHandlerFn = fn;
  signal(SIGINT, SignalManager::signalHandler);
  signal(SIGQUIT, SignalManager::signalHandler);
}

string parse_tilde_in_filepath(string filepath) {
  if (util::starts_with(filepath, "~/")) {
    return filepath.replace(0, 1, string(getenv("HOME")));
  }

  return filepath;
}

string util::fread(string filepath) {
  filepath = parse_tilde_in_filepath(filepath);
  FILE *file = ::fopen(filepath.c_str(), "r");

  if (!file) {
    throw system_error();
  }

  // get file size
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);

  // read file
  char *contents_char = new char[size];
  rewind(file);
  fread(contents_char, sizeof(char), size, file);

  // convert to string
  string contents(contents_char);

  // cleanup
  delete[] contents_char;
  fclose(file);

  return contents;
}

bool util::fwrite(string filepath, string contents) {
  parse_tilde_in_filepath(filepath);
  FILE *file = ::fopen(filepath.c_str(), "w");

  if (!file) {
    throw system_error();
  }

  unsigned int n_written = fwrite(contents.c_str(), sizeof(char), contents.size(), file);
  fclose(file);

  return n_written == contents.size();
}
