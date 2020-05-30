#pragma once
#include "EventLoop.h"
#include "Thread.h"
#include <mutex>
#include <condition_variable>
#include <iostream>
class EventLoopThread {
public:
    EventLoopThread() : m_pLoop(nullptr), m_oThread(std::bind(&EventLoopThread::loop, this)) {}
    EventLoop* startLoop() {
        m_oThread.start();
        {
            std::unique_lock<std::mutex> lock(m_mMutex);
            while (m_pLoop == nullptr) {
                m_oCond.wait(lock);
            }
        }
        return m_pLoop;
    }

    void loop() {
        EventLoop oLoop;
        {
            std::unique_lock<std::mutex> lock(m_mMutex);
            m_pLoop = &oLoop;
            m_oCond.notify_one();
        }
        m_pLoop->loop();
        std::unique_lock<std::mutex> lock(m_mMutex);
        m_pLoop = nullptr;
    }
private:
    EventLoop* m_pLoop;
    std::mutex m_mMutex;
    std::condition_variable m_oCond;
    Thread m_oThread;
};