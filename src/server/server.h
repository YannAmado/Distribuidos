#ifndef SERVER_H
#define SERVER_H

#include "user.h"
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <errno.h>
#include <mutex>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

using namespace std;

#define PING_CMD "/ping"
#define KICK_CMD "/kick"
#define MUTE_CMD "/mute"
#define UNMUTE_CMD "/unmute"
#define WHOIS_CMD "/whois"

class Server {
private:
  /** Send a direct message to a user */
  void dm(User *target, string message);
  void listen_client(User *user);
  void disconnect(User *user);
  void close_channel(Channel *channel);
  void log(string message);

public:
  void launch();
};

#endif
