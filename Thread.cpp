#include "Thread.h"

struct ThreadData {

};
bool Thread::joined() {
    return m_bJoined;
}

bool Thread::started() {
    return m_bStarted;
}

void* startThread(void* apoTask) {
    Thread::Task* poTask = static_cast<Thread::Task*>(apoTask);
    (*poTask)();
    return NULL;
}

int Thread::start() {
    m_bStarted = true;
    return pthread_create(&m_tThreadId, NULL, &startThread, &m_oTask);
}

Thread::~Thread() {
    if (m_bStarted && !m_bJoined) {
        pthread_detach(m_tThreadId);
    }
}

int Thread::join() {
    assert(m_bStarted);
    assert(!m_bJoined);
    m_bJoined = true;
    return pthread_join(m_tThreadId, NULL);
}

Thread::Thread(Task aoTask) {
    m_oTask = aoTask;
}