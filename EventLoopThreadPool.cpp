#include "EventLoopThreadPool.h"
void EventLoopThreadPool::start() {
    for (int i = 0; i < m_iNumThreads; ++i) {
        EventLoopThread* pThread = new EventLoopThread();
        m_vecThreads.push_back(std::unique_ptr<EventLoopThread>(pThread));
        EventLoop* pLoop = pThread->startLoop();
        m_vecLoops.push_back(pLoop);
    }
    m_bRunning = true;
}

EventLoop* EventLoopThreadPool::getNextLoop() {
    EventLoop* pLoop = m_pBaseLoop;
    if (m_vecLoops.empty()) {
        return pLoop;
    }
    
    pLoop = m_vecLoops[m_iNextLoopIdx];
    m_iNextLoopIdx = (m_iNextLoopIdx + 1) % m_vecLoops.size();
    return pLoop;
}

void EventLoopThreadPool::shutdown() {
    for (auto& pLoop : m_vecLoops) {
        pLoop->quit();
    }
    m_bRunning = false;
}