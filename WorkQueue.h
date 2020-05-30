#include "pthread.h"
#include "Thread.h"
#include <cassert>
#include <deque>

class WorkQueue {
private:
    std::deque<Thread::Task> m_lstWorkItems;
    pthread_cond_t m_oNotEmpty = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t m_oMutex = PTHREAD_MUTEX_INITIALIZER;
    bool m_bRunning;
public:
    WorkQueue() : m_bRunning(true) {}
    void push(Thread::Task aoTask);
    Thread::Task pop();
    void shutdown() {
        if (m_bRunning) {
            m_bRunning = false;
            pthread_cond_broadcast(&m_oNotEmpty);
        }
    }
    bool running() {
        return m_bRunning;
    }
};