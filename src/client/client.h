#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <thread>
#include <vector>

#define CONNECT_CMD "/connect"
#define USER_PROMPT "You: "
#define SERVER_PROMPT "-> "

using namespace std;

class Client {
private:
  int server_socket;
  vector<thread> thrs;

  vector<string> split_chunks(string message);
  string recv();
  void send(string message);
  void erase_text(int len);
  void send_message_handler();
  void recv_message_handler();
  void do_teardown();

public:
  void launch();
  static void warn_teardown();
};

#endif
