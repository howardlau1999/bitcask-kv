#include "Channel.h"

void Channel::handleEventWithGuard() {
    m_bHandlingEvents = true;
    
    if ((m_iReceivedEvents & EPOLLHUP) && !(m_iReceivedEvents & EPOLLIN)) {
        if (m_funcCloseCallback) m_funcCloseCallback();
    }

    if (m_iReceivedEvents & (EPOLLERR)) {
        if (m_funcCloseCallback) m_funcCloseCallback();
        m_bHandlingEvents = false;
        return;
    }

    if (m_iReceivedEvents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (m_funcReadCallback) m_funcReadCallback();
    }

    if (m_iReceivedEvents & EPOLLOUT) {
        if (m_funcWriteCallback) m_funcWriteCallback();
    }

    m_bHandlingEvents = false;
}