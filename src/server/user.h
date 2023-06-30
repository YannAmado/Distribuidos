#ifndef USER_H
#define USER_H

#include <string>
#include <thread>
#include <vector>

using namespace std;

class User {
  private:
    bool muted;
    int socket;

    static vector<User> users;
    static int max_id;
  public:
    int id;
    string name;
    thread thr;
    bool adm;
    string channel;
    char *ip;

    User(int socket, char *ip);

    void send(string message);
    void mute();
    void unmute();
    bool is_mute();

    int get_socket();

    static User *get_by_id(int id);
    static int get_num_users();
};

#endif
