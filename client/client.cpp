#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <iostream>
#include <cstring>

using namespace std;

string message;
char buf[1024];
struct hostent *server;

int readn(int fd, char *bp, size_t len){
    int cnt;
    int rc;

    cnt = len;
    while(cnt > 0){
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

int main(int argc, char *argv[]) {    
    int sock;
    struct sockaddr_in addr{};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Can not open socket");
        exit(1);
    }
    server = gethostbyname(argv[1]);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    bcopy(server->h_addr, (char *) &addr.sin_addr.s_addr, (size_t) server->h_length);
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Failed to connect to server");
        exit(1);
    }
    readn(sock, buf, 1024);
    cout << buf << "\n";
    while(1){
        getline(cin, message);
        if (send(sock, message.c_str(), 1024, 0) <= 0){
            break;
        }
        if (readn(sock, buf, 1024) <= 0) {
            break;
        }
        cout << buf << "\n";
    }
    shutdown(sock, 2);
    close(sock);
    return 0;
}
