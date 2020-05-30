#include "WorkQueue.h"

void WorkQueue::push(Thread::Task aoTask) {
    pthread_mutex_lock(&m_oMutex);
    m_lstWorkItems.push_back(std::move(aoTask));
    pthread_cond_signal(&m_oNotEmpty);
    pthread_mutex_unlock(&m_oMutex);
}

Thread::Task WorkQueue::pop() {
    pthread_mutex_lock(&m_oMutex);
    while (m_lstWorkItems.empty() && m_bRunning) {
        pthread_cond_wait(&m_oNotEmpty, &m_oMutex);
    }
    Thread::Task oFunc;
    if (!m_lstWorkItems.empty()) {
        oFunc = std::move(m_lstWorkItems.front());
        m_lstWorkItems.pop_front();
    }
    pthread_mutex_unlock(&m_oMutex);
    return oFunc;
}