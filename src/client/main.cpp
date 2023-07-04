#include "../util.h"
#include "client.h"

int main() {
  Client client;

  util::SignalManager::init([&client](int signal) { Client::warn_teardown(); });
  client.launch();

  return 0;
}
