#pragma once
#include "Channel.h"
#include "Socket.h"
#include <iostream>
#include <functional>

class Acceptor {
public:
    typedef std::function<void(int sockfd, const struct sockaddr_storage)>
        NewConnectionCallback;

    Acceptor(EventLoop* apLoop, const struct addrinfo* apAddr);

    bool isListening() { return this->m_bListening; }

    void setNewConnectionCallback(NewConnectionCallback aCallback) {
        this->m_funcNewConnectionCallback = std::move(aCallback);
    }

    void handleRead();

    ~Acceptor();

    void listen();

private:
    EventLoop* m_pLoop;
    Socket m_oAcceptSocket;
    Channel m_oAcceptChannel;
    bool m_bListening;
    NewConnectionCallback m_funcNewConnectionCallback;
};