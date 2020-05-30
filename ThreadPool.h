#pragma once
#include "Thread.h"
#include "WorkQueue.h"
#include <vector>
#include <memory>

class ThreadPool {
public:
    void start(int aiThreadNum) {
        m_vecThreads.reserve(aiThreadNum);
        for (int i = 0; i < aiThreadNum; ++i) {
            m_vecThreads.emplace_back(new Thread(std::bind(&ThreadPool::work, this)));
            m_vecThreads[i]->start();
        }
        m_bRunning = true;
    }

    void shutdown() {
        if (m_bRunning) {
            m_bRunning = false;
            m_oWorkQueue.shutdown();
        }
    }

    void joinAll() {
        for (auto& poThread : m_vecThreads) {
            poThread->join();
        }
    }

    void work() {
        while (m_bRunning) {
            auto oTask = m_oWorkQueue.pop();
            if (oTask) oTask();
        }
    }

    void submit(Thread::Task aoTask) {
        m_oWorkQueue.push(aoTask);
    }

    bool running() {
        return m_bRunning;
    }

    ~ThreadPool() {
        if (m_bRunning) {
            m_bRunning = false;
            m_oWorkQueue.shutdown();
        }
    }
private:
    bool m_bRunning;
    std::vector<std::unique_ptr<Thread>> m_vecThreads;
    WorkQueue m_oWorkQueue;
};