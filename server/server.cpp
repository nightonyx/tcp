#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <list>
#include <thread>
#include <iostream>
#include <sstream>
#include <cstring>

using namespace std;

int listener;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
list <string> threads;
list <thread> thread_list;

int readn(int fd, char *bp, size_t len) {
    int cnt;
    int rc;

    cnt = len;
    while (cnt > 0) {
        rc = recv(fd, bp, cnt, 0);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (rc == 0)
            return len - cnt;
        bp += rc;
        cnt -= rc;
    }
    return len;
}

string listToString() {
    string listThreads = "list threads: ";
    for (const string &s : threads) {
        listThreads += s;
        listThreads += " ";
    }
    return listThreads;
}

list <string> split(const string &s, char delimiter) {
    list <string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

string toStr(thread::id id) {
    stringstream ss;
    ss << id;
    return ss.str();
}

void exit() {
    shutdown(listener, 2);
    close(listener);
}

bool containsThread(const string &id) {
    for (const string &elem : threads) {
        if (strcmp(elem.c_str(), id.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

void kill(const string &id, string &mString) {
    if (containsThread(id)) {
        threads.remove(id);
        mString = "killed ";
        mString += id;
    } else {
        mString = "Can not kill this";
    }
}

void fun(int sock) {
    string hello = "Hello ";
    hello += toStr(this_thread::get_id());
    send(sock, hello.c_str(), 1024, 0);
    while (true) {
        string message;
        char buf[1024];
        int bytes_read;


        if (!containsThread(toStr(this_thread::get_id()))) {
            break;
        }

        bytes_read = readn(sock, buf, 1024);
        if (bytes_read <= 0) {
            break;
        }

        if (!containsThread(toStr(this_thread::get_id()))) {
            break;
        }
        cout << buf << "\n";

        if (strcmp(buf, string("list").c_str()) == 0) {
            message = listToString();
        } else if (strcmp(buf, string("exit").c_str()) == 0) {
            exit();
        } else {
            list <string> listSplit = split(buf, ' ');
            if (strcmp(listSplit.front().c_str(), string("kill").c_str()) == 0) {
                listSplit.pop_front();
                kill(listSplit.front(), message);
            } else {
                message = "Message: " + string(buf);
            }
        }
        send(sock, message.c_str(), 1024, 0);

        if (!containsThread(toStr(this_thread::get_id()))) {
            break;
        }
    }
    shutdown(sock, 2);
    close(sock);
    pthread_mutex_lock(&mutex);
    threads.remove(toStr(this_thread::get_id()));
    pthread_mutex_unlock(&mutex);
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in addr{};
    listener = socket(AF_INET, SOCK_STREAM, 0);

    if (listener < 0) {
        perror("Can not create socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    const int on = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);

    if (bind(listener, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Failed to bind socket with address");
        exit(1);
    }

    listen(listener, 5);

    while (true) {
        sock = accept(listener, nullptr, nullptr);

        if (sock < 0) {
            perror("Failed to accept");
            exit(1);
        }

        thread_list.emplace_back([&] { fun(sock); });
        pthread_mutex_lock(&mutex);
        threads.push_back(toStr(thread_list.back().get_id()));
        pthread_mutex_unlock(&mutex);

    }

    for (thread &t: thread_list) {
        t.join();
    }

}
