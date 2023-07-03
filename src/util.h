#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sys/socket.h>
#include <vector>

#define JOIN_CMD "/join"
#define QUIT_CMD "/quit"

#define MAX_STR 4096
#define BYTES_PER_MSG 4096
#define MAX_CHAR_NAME 50
#define MAX_USERS_PER_CHANNEL 50
#define INVALID -1
#define KICK_SIG "#KICK"
#define ERROR_SIG "#ERROR"
#define SUCCESS_SIG "#SUCCESS"
#define TERMINATE_SIG "#TERMINATE"

using namespace std;

namespace util {
void send(int socket, string message);
string recv(int socket);
string get_ip(const struct sockaddr *sa);
bool starts_with(string str, string prefix);
bool is_name_valid(string name);
} // namespace util

#endif
