#include "Socket.h"
#include <iostream>

Socket::~Socket() { close(fd); }

void Socket::bindAddress(const struct sockaddr* addr) {
  bindOrDie(fd, addr);
}

void Socket::listen() {
  listenOrDie(fd);
}

int Socket::accept(struct sockaddr_in6 *clientAddr) {
    bzero(clientAddr, sizeof *clientAddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof *clientAddr);
    int connfd = ::accept4(fd, sockaddrCast(clientAddr), &addrlen,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0) {
        int savedErrno = errno;
        std::cerr << "sockets::accept";
        switch (savedErrno) {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:
            case EPERM:
            case EMFILE:
                errno = savedErrno;
                exit(1);
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                std::cerr << "unexpected error of ::accept " << savedErrno;
                exit(1);
                break;
            default:
                std::cerr << "unknown error of ::accept " << savedErrno;
                exit(1);
                break;
        }
    }
    return connfd;
}

int getInetPort(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in *)sa)->sin_port);
    }

    return (((struct sockaddr_in6 *)sa)->sin6_port);
}

void *getInetAddr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


void bindOrDie(int sockfd, const struct sockaddr* addr) {
    int ret = ::bind(sockfd, addr,
                     static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if (ret < 0) {
        perror("sockets::bindOrDie");
        exit(1);
    }
}

void listenOrDie(int sockfd) {
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0) {
        std::cerr << "sockets::listenOrDie";
    }
}

struct sockaddr* sockaddrCast(struct sockaddr_in6* addr) {
    return static_cast<struct sockaddr*>(
        reinterpret_cast<void*>(addr));
}

int listenPort(const char* ip, const char *port) {
    struct addrinfo hints, *servinfo, *p;

    const int BACKLOG = MAX_CONNS;
    int rv, fd;
    int yes = 1;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket error");
            continue;
        }

        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("server: setsockopt error");
            exit(1);
        }

        if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(fd);
            perror("server: bind error");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "server: failed to listen\n");
        exit(1);
    }

    if (listen(fd, BACKLOG) == -1) {
        close(fd);
        perror("server: listen error");
        exit(1);
    }

    return fd;
}

void recvN(int fd, char *buf, const int size) {
    int received = 0;
    while (received < size) {
        int chunk = recv(fd, buf, size - received, 0);
        if (chunk == -1) {
            perror("recv");
            return;
        }
        received += chunk;
        buf += chunk;
    }
}

void sendN(int fd, const char *buf, const int size) {
    int sent = 0;
    while (sent < size) {
        int chunk = send(fd, buf, size - sent, 0);
        if (chunk == -1) {
            perror("send");
            return;
        }
        sent += chunk;
        buf += chunk;
    }
}

int createNonblockingOrDie(sa_family_t family) {
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                          IPPROTO_TCP);
    if (sockfd < 0) {
        perror("sockets::createNonblockingOrDie");
        exit(1);
    }
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        perror("setsockopt");
        exit(1);
    }
    return sockfd;
}

void setNonBlocking(int fd) {
    int opts;
    if ((opts = fcntl(fd, F_GETFL)) < 0) {
        fprintf(stderr, "GETFL failed");
        exit(1);
    }
    opts |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, opts) < 0) {
        fprintf(stderr, "SETFL failed");
        exit(1);
    }
}

int connectTo(const char* host, const char *port) {
    struct addrinfo hints, *servinfo, *p;
    int rv, fd;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((fd = socket(p->ai_family, p->ai_socktype | SOCK_CLOEXEC, p->ai_protocol)) == -1) {
            perror("client: socket error");
            continue;
        }

        if (connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(fd);
            perror("client: connect error");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(2);
    }

    inet_ntop(p->ai_family, getInetAddr((struct sockaddr *)&p->ai_addr), s,
              sizeof s);
    fprintf(stdout, "client: sockfd %d connected to %s:%s\n", fd, s, port);
    freeaddrinfo(servinfo);
    setNonBlocking(fd);
    return fd;
}
