#include "TcpConnection.h"

TcpConnection::TcpConnection(EventLoop* aLoop, int aSockFd, int aConnId)
    : m_iSockFd(aSockFd),
      m_iConnId(aConnId),
      m_pLoop(aLoop),
      m_pSocket(new Socket(aSockFd)),
      m_pChannel(new Channel(aLoop, aSockFd)) {

    m_pChannel->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    m_pChannel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    m_pChannel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));

};

void TcpConnection::send(Buffer* apBuffer) {
    if (m_pLoop->isInLoopThread()) {
        sendInLoop(apBuffer->peek(), apBuffer->readableBytes());
        apBuffer->retrieveAll();
    } else {
      void (TcpConnection::*pFunc)(std::string aMessage) = &TcpConnection::sendInLoop;
      m_pLoop->runInLoop(
          std::bind(pFunc,
                    this,
                    apBuffer->retrieveAllAsString()));
    }
}

void TcpConnection::sendInLoop(std::string aMessage) {
    sendInLoop(&*aMessage.begin(), aMessage.size());
}

void TcpConnection::sendInLoop(const void* apBuf, int aLen) {
    m_pLoop->assertInLoopThread();

    const char* pBuffer = reinterpret_cast<const char*>(apBuf);
    int iBytesWritten = 0;
    int iRemaining = aLen;

    if (!m_pChannel->isWriting() && m_oOutputBuffer.readableBytes() == 0) {
        iBytesWritten = ::write(m_pChannel->getFd(), pBuffer, aLen);
        if (iBytesWritten >= 0) {
            iRemaining = aLen - iBytesWritten;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("write");
            handleClose();
        }
    }

    if (iRemaining > 0) {
        m_oOutputBuffer.append(pBuffer + iBytesWritten, iRemaining);
        if (!m_pChannel->isWriting()) {
            m_pChannel->enableWriting();
        }
    }
}

void TcpConnection::handleClose() {
    m_pChannel->disableAll();
    
    std::shared_ptr<TcpConnection> guard(shared_from_this());
    if (m_funcCloseCallback) {
        m_funcCloseCallback(shared_from_this());
    }
}

void TcpConnection::handleWrite() {
    if (m_pChannel->isWriting()) {
        int n = ::write(m_pChannel->getFd(), m_oOutputBuffer.peek(),
                               m_oOutputBuffer.readableBytes());
        if (n > 0) {
            m_oOutputBuffer.retrieve(n);
            if (m_oOutputBuffer.readableBytes() == 0) {
                m_pChannel->disableWriting();
            }
        } else {
            perror("handleWrite");
        }
    }
}

void TcpConnection::handleRead() {
    int n = m_oInputBuffer.readFd(getFd(), nullptr);
    if (n > 0) {
        if (m_funcMessageCallback) {
            m_funcMessageCallback(shared_from_this());
        }
    } else {
        handleClose();
    }
}