#include "../util.h"
#include "server.h"

int main() {
  Server server;

  util::SignalManager::init([&server](int signal) {
    server.teardown();
    util::SignalManager::init(SIG_DFL);
  });
  server.launch();

  return 0;
}
