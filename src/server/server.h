#ifndef SERVER_H
#define SERVER_H

#include "user.h"

#define PING_CMD "/ping"
#define KICK_CMD "/kick"
#define MUTE_CMD "/mute"
#define UNMUTE_CMD "/unmute"
#define WHOIS_CMD "/whois"

class Server {
private:
  int socket;

  void send(User *target, string message);
  void listen_client_handler(User *user);
  void disconnect(User *user);
  void close_channel(Channel *channel);
  void log(string message);
  void err(string message);

public:
  void launch();
  void teardown();
};

#endif
