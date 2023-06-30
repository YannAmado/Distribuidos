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

class Server {
private:
  bool is_client_adm(int client_id);

public:
  /** Send a direct message to a user */
  void dm(User target, string message);
  void send_dm(int num, int target_id);
  void set_client_name(int id);
};

#endif
