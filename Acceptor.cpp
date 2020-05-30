#include "Acceptor.h"

Acceptor::Acceptor(EventLoop* apLoop, const struct addrinfo* apAddr)
    : m_pLoop(apLoop), m_oAcceptSocket(createNonblockingOrDie(apAddr->ai_family)),
        m_oAcceptChannel(apLoop, m_oAcceptSocket.getFd()),
        m_bListening(false) {
    m_oAcceptSocket.bindAddress(apAddr->ai_addr);
    m_oAcceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}


void Acceptor::handleRead() {
    struct sockaddr_storage clientAddr;
    int iConnfd = m_oAcceptSocket.accept(reinterpret_cast<sockaddr_in6*>(&clientAddr));
    if (iConnfd < 0) {
        std::cerr << "Cannot accept" << std::endl;
    } else {
        if (m_funcNewConnectionCallback) {
            m_funcNewConnectionCallback(iConnfd, clientAddr);
        } else {
            ::close(iConnfd);
        }
    }
}

Acceptor::~Acceptor() {
    m_oAcceptChannel.disableAll();
    m_oAcceptChannel.remove();
}

void Acceptor::listen() {
    m_pLoop->assertInLoopThread();
    m_bListening = true;
    m_oAcceptSocket.listen();
    m_oAcceptChannel.enableReading();
}