#ifndef UTIL_H
#define UTIL_H

#include <functional>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <vector>

#define JOIN_CMD "/join"
#define QUIT_CMD "/quit"
#define NICKNAME_CMD "/nickname"
#define FILE_CMD "/file"

#define MAX_STR 4096
#define BYTES_PER_MSG 4096
#define MAX_CHAR_NAME 50
#define MAX_USERS_PER_CHANNEL 50
#define INVALID -1

#define SIG_KICK "#KICK"
#define SIG_ERROR "#ERROR"
#define SIG_SUCCESS "#SUCCESS"
#define SIG_TERMINATE "#TERMINATE"
#define SIG_MESSSAGE "#MESSAGE"
#define SIG_NOTIF "#NOTIFICATION"
#define SIG_FILE "#FILE"

using namespace std;

namespace util {
  void send(int socket, string message);
  void send_without_filtering(int socket, string message);
  string recv(int socket);
  string get_ip(const sockaddr *sa);
  bool starts_with(string str, string prefix);
  bool is_name_valid(string name);
  string get_cmd_target(string message, string cmd);
  string fread(string filepath);
  bool fwrite(string filepath, string contents);

  struct SignalManager {
    private:
      static inline function<void(int)> signalHandlerFn;
      static void signalHandler(int signal);

    public:
      static void init(std::function<void(int)> fn);
  };
} // namespace util

#endif
