#pragma once
#include "EventLoop.h"
#include "Socket.h"
#include <functional>
class EventLoop;
class Channel {
   public:
    typedef std::function<void()> CallbackFn;
    Channel(EventLoop* apLoop, int aFd) : 
        m_pLoop(apLoop), m_iFd(aFd), m_iIndex(-1), 
        m_iEvents(0), m_iReceivedEvents(0), 
        m_bTied(false), m_bHandlingEvents(false) {}

    int getFd() { 
        return m_iFd; 
    }

    int getEvents() { 
        return this->m_iEvents; 
    }

    int getReceivedEvents() { 
        return this->m_iReceivedEvents; 
    }

    void setReceivedEvents(int aReceivedEvents) { 
        this->m_iReceivedEvents = aReceivedEvents; 
    }

    int getIndex() { 
        return m_iIndex; 
    }

    void setIndex(int aIndex) { 
        this->m_iIndex = aIndex; 
    }
    void handleEventWithGuard();

    void setReadCallback(CallbackFn aCallback) { 
        m_funcReadCallback = std::move(aCallback); 
    }

    void setWriteCallback(CallbackFn aCallback) { 
        m_funcWriteCallback = std::move(aCallback); 
    }

    void setCloseCallback(CallbackFn aCallback) { 
        m_funcCloseCallback = std::move(aCallback); 
    }

    void setErrorCallback(CallbackFn aCallback) { 
        m_funcErrorCallback = std::move(aCallback); 
    }
    void enableReading() { 
        this->m_iEvents |= (EPOLLIN | EPOLLPRI); update(); 
    }
    void disableReading() { 
        this->m_iEvents &= ~(EPOLLIN | EPOLLPRI); update(); 
    }
    void enableWriting() { 
        this->m_iEvents |= (EPOLLOUT); update(); 
    }
    void disableWriting() { 
        this->m_iEvents &= ~(EPOLLOUT); update(); 
    }
    void disableAll() { 
        this->m_iEvents = 0; update(); 
    }
    bool isWriting() { 
        return (this->m_iEvents & EPOLLOUT) == EPOLLOUT;
    }
    bool isReading() { 
        return (this->m_iEvents & EPOLLIN) == EPOLLIN;
    }
    bool isNoneEvents() { 
        return this->m_iEvents == 0; 
    }
    void update() { 
        m_pLoop->updateChannel(this); 
    }
    void remove() { 
        m_pLoop->removeChannel(this); 
    }
    EventLoop* getEventLoop() { 
        return m_pLoop; 
    }

    void handleEvent() {
        std::shared_ptr<void> pGuard;
        if (m_bTied) {
            pGuard = m_pTie.lock();
            if (pGuard) {
                handleEventWithGuard();
            }
        } else {
            handleEventWithGuard();
        }   
    }

    void tie(std::shared_ptr<void> const& apObj) {
        m_pTie = apObj;
        m_bTied = true;
    }
private:
    EventLoop* m_pLoop;
    int m_iFd;
    int m_iIndex;
    int m_iEvents;
    int m_iReceivedEvents;
    std::weak_ptr<void> m_pTie;
    bool m_bTied;
    bool m_bHandlingEvents;
    CallbackFn m_funcReadCallback, m_funcWriteCallback, m_funcCloseCallback, m_funcErrorCallback;
};