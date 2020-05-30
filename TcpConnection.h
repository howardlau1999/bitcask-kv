#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include <deque>
#include "Buffer.h"
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    typedef std::function<void(std::shared_ptr<TcpConnection> const& conn)>
        MessageCallback;
    typedef std::function<void(std::shared_ptr<TcpConnection> const& conn)> CloseCallback;
    
    TcpConnection(EventLoop* aLoop, int aSockFd, int aConnId);

    int getFd() { 
        return m_iSockFd; 
    }

    EventLoop* getLoop() { 
        return m_pLoop; 
    }

    int getConnId() {
        return m_iConnId; 
    }

    void send(Buffer* buf);

    void sendInLoop(std::string message);

    void sendInLoop(const void* buffer, const int len);

    void handleRead();

    void handleWrite();

    void handleClose();

    void setMessageCallback(MessageCallback const& aCallback) { 
        m_funcMessageCallback = aCallback; 
    }
    void setCloseCallback(CloseCallback const& aCallback) { 
        m_funcCloseCallback = aCallback; 
    }

    void connectionEstablished() { 
        m_pChannel->tie(shared_from_this());
        m_pChannel->enableReading(); 
    }

    Buffer& getInputBuffer() { 
        return this->m_oInputBuffer; 
    }

    void connectionDestroyed() {
        m_pChannel->disableAll();
        m_pChannel->remove();
    }

private:
    int m_iSockFd;
    int m_iConnId;
    EventLoop* m_pLoop;
    Buffer m_oInputBuffer;
    Buffer m_oOutputBuffer;
    std::unique_ptr<Socket> m_pSocket;
    std::unique_ptr<Channel> m_pChannel;
    MessageCallback m_funcMessageCallback;
    CloseCallback m_funcCloseCallback;
};
