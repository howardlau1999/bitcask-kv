#pragma once
#include "EventLoop.h"
#include "EventLoopThread.h"
#include <vector>
class EventLoopThreadPool {
public:
    EventLoopThreadPool(int aiNumThreads, EventLoop* pBaseLoop) : m_pBaseLoop(pBaseLoop), m_iNextLoopIdx(0), m_iNumThreads(aiNumThreads), m_bRunning(false) {} 
    void shutdown();
    void start();
    EventLoop* getNextLoop();
private:
    std::vector<EventLoop*> m_vecLoops;
    std::vector<std::unique_ptr<EventLoopThread>> m_vecThreads;
    EventLoop* m_pBaseLoop;
    int m_iNextLoopIdx;
    int m_iNumThreads;
    bool m_bRunning;
};