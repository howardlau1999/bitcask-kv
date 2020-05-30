#pragma once
#include "pthread.h"
#include <cassert>
#include <functional>

class Thread {
public:
    typedef std::function<void()> Task;
    explicit Thread(Task aFunc);
    ~Thread();
    int start();
    int join();
    bool started();
    bool joined();
private:
    pthread_t m_tThreadId;
    pid_t m_tTid;
    bool m_bStarted;
    bool m_bJoined;
    Task m_oTask;
};