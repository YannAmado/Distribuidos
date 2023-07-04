#include "client.h"
#include "../util.h"
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <errno.h>
#include <mutex>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

using namespace std;

void Client::send(string message) { util::send(server_socket, message); }

string Client::recv() { return util::recv(server_socket); }

void Client::erase_text(int len) {
  cout << "\r";
  for (int i = 0; i < len; i++) {
    cout << " ";
  }
  cout << "\r";
}

vector<string> Client::split_chunks(string message) {
  vector<string> chunks;
  chunks.reserve(ceil(message.size() / MAX_STR));

  while (message.size() > MAX_STR) {
    chunks.push_back(message.substr(0, MAX_STR));
    message = message.substr(MAX_STR);
  }

  if (message.size() > 0) {
    chunks.push_back(message);
  }

  return chunks;
}

void Client::send_message_handler() {
  string message;

  while (true) {
    cout << USER_PROMPT;
    getline(cin, message);

    if (!cin || cin.eof() || cin.bad()) {
      send(QUIT_CMD);
    } else if (util::starts_with(message, FILE_CMD)) {
      string filepath = util::get_cmd_target(message, FILE_CMD);
      string contents;

      cout << SERVER_PROMPT << "Uploading file '" << filepath
           << "' to server..." << endl;

      try {
        contents = util::fread(filepath);
      } catch (...) {
        cout << SERVER_PROMPT << "File doesn't exist or is invalid." << endl;
        continue;
      }

      cout << SERVER_PROMPT << "'" << filepath << "' has "
           << to_string(contents.size()) << " bytes." << endl;

      util::send_without_filtering(server_socket,
                                   (string)FILE_CMD + " " + contents);
    } else {
      vector<string> message_chunks = split_chunks(message);

      for (string &chunk : message_chunks) {
        send(chunk);
      }
    }
  }
}

void Client::recv_message_handler() {
  while (true) {
    string sig = recv();
    erase_text(((string)USER_PROMPT).size());

    if (sig == SIG_MESSSAGE) {
      string sender = recv(), message = recv();
      cout << sender << ": " << message << endl;
    } else if (sig == SIG_NOTIF) {
      cout << SERVER_PROMPT << recv() << endl;
    } else if (sig == SIG_ERROR) {
      cout << SERVER_PROMPT << "Error from server: " << recv() << endl;
    } else if (sig == SIG_SUCCESS) {
      string cmd = recv(), message = recv();
      cout << SERVER_PROMPT << message << endl;

      if (cmd == QUIT_CMD) {
        do_teardown();
      }
    } else if (sig == SIG_KICK) {
      cout << SERVER_PROMPT << recv() << endl;
    } else if (sig == SIG_TERMINATE) {
      cout << SERVER_PROMPT << "Server is shutting down." << endl;
      cout << SERVER_PROMPT << "Quitting..." << endl;
      do_teardown();
    } else if (sig == SIG_FILE) {
      string sender = recv(), contents = recv();
      string filepath = string(getenv("HOME")) + "/Downloads/" + sender + "-" +
                        to_string(time(0)) + ".txt";

      cout << SERVER_PROMPT << sender << " sent a file with "
           << to_string(contents.size()) << " bytes. Downloading it to '"
           << filepath << "'..." << endl;

      bool downloaded_all = false;

      try {
        downloaded_all = util::fwrite(filepath, contents);
      } catch (...) {
        cout << SERVER_PROMPT
             << "There was an error, couldn't download file to '" << filepath
             << "'." << endl;
        continue;
      }

      if (!downloaded_all) {
        cout << SERVER_PROMPT
             << "There was an error, couldn't download the whole file to '"
             << filepath << "'." << endl;
        continue;
      }

      cout << SERVER_PROMPT << "File successfully downloaded to '" << filepath
           << "'." << endl;
    }

    cout << USER_PROMPT;
    fflush(stdout);
  }
}

void Client::launch() {
  sockaddr_in server_sockaddr;
  server_sockaddr.sin_family = AF_INET;
  server_sockaddr.sin_port = htons(9002);
  server_sockaddr.sin_addr.s_addr = INADDR_ANY;
  server_socket = socket(server_sockaddr.sin_family, SOCK_STREAM, 0);

  cout << "\n"
       << SERVER_PROMPT << "Send '" << CONNECT_CMD
       << "' if you wanna connect to the server or '/quit' "
          "if you wanna exit."
       << endl;

  int server_connection;

  do {
    string cmd;
    getline(cin, cmd);

    if (cmd == QUIT_CMD || !cin || cin.eof() || cin.bad()) {
      cout << "Quitting..." << endl;
      send(QUIT_CMD);
      close(server_socket);
      exit(0);
    } else if (cmd == CONNECT_CMD) {
      server_connection = connect(server_socket, (sockaddr *)&server_sockaddr,
                                  sizeof(sockaddr_in));
      string result = recv();

      if (server_connection == INVALID) {
        cerr << "[ERROR] Couldn't connect to server." << endl;
        exit(EXIT_FAILURE);
      } else if (result == SIG_ERROR) {
        cerr << "[ERROR] Server: " << recv() << endl;
        exit(EXIT_FAILURE);
      } else if (result == SIG_SUCCESS) {
        cout << SERVER_PROMPT << recv() << endl;
      }
    } else {
      cout << SERVER_PROMPT << "Invalid command." << endl;
      server_connection = INVALID;
    }
  } while (server_connection == INVALID);

  thread thr_send(&Client::send_message_handler, this);
  thread thr_recv(&Client::recv_message_handler, this);

  thrs.push_back(::move(thr_send));
  thrs.push_back(::move(thr_recv));

  for (auto &thr : thrs) {
    if (thr.joinable()) {
      thr.join();
    }
  }
}

void Client::do_teardown() {
  cout << "\nTearing down..." << endl;

  for (auto &thr : thrs) {
    thr.detach();
  }

  close(server_socket);
  exit(EXIT_SUCCESS);
}

void Client::warn_teardown() {
  cout << "\n"
       << SERVER_PROMPT
       << "Ctrl+C is deactivated. Please press Ctrl+D or send '" << QUIT_CMD
       << "' to exit safely" << endl;
}
