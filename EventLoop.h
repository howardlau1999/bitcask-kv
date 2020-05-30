#pragma once
#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <sys/eventfd.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include "CurrentThread.h"
class Channel;
class Poller;
class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void assertInLoopThread() {
        if (!isInLoopThread()) {
            abortNotInLoopThread();
        }
    }

    void abortNotInLoopThread() {
        char arrBuf[1024] = {0};
        snprintf(arrBuf, sizeof(arrBuf), "EventLoop::abortNotInLoopThread - EventLoop %p was created in threadId_ = %d, current thread id =  %d\n",
                this, m_iThreadId, CurrentThread::tid());
        write(STDERR_FILENO, arrBuf, strlen(arrBuf));
        abort();
    }

    void loop();
    void quit();
    bool isInLoopThread();
    void wakeup();
    void doPendingFunctors();
    void handleRead();
    void queueInLoop(std::function<void()> cb);

    void runInLoop(std::function<void()> cb);
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    
    const int getThreadId() {
        return m_iThreadId;
    }


private:
    std::vector<Channel*> m_vecActiveChannels;
    std::vector<std::function<void()>> m_vecPendingFunctors;
    std::unique_ptr<Poller> m_pPoller;
    std::mutex m_mMutex;
    Channel* m_pCurrentActiveChannel;
    bool m_bHandlingEvents;
    bool m_bCallingPendingFunctors;
    bool m_bQuit;
    bool m_bLooping;
    const pid_t m_iThreadId;

    int m_iEventFd;
    std::unique_ptr<Channel> m_pWakeupChannel;
};