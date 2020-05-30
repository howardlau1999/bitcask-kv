#pragma once
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_LEN 4096
#define MAX_WINDOW 4096
#define MAX_EVENTS 32
#define MAX_CONNS 32

class Socket {
   private:
    int fd;

   public:
    int accept(struct sockaddr_in6 *clientAddr);
    void bindAddress(const struct sockaddr* addr);
    void listen();
    int getFd() const { return fd; }
    explicit Socket(int fd) : fd(fd) {}
    ~Socket();
};

int connectTo(const char *ip, const char *port);
int listenPort(const char *ip, const char *port);
void recvN(int fd, char *buf, const int size);
void sendN(int fd, const char *buf, const int size);
int createNonblockingOrDie(sa_family_t family);
struct sockaddr* sockaddrCast(struct sockaddr_in6* addr);
void listenOrDie(int sockfd);
void bindOrDie(int sockfd, const struct sockaddr* addr);


int getInetPort(struct sockaddr *sa);

void *getInetAddr(struct sockaddr *sa);