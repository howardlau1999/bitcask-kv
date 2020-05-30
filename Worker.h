#pragma once
#include "Thread.h"
#include "WorkQueue.h"

class Worker {
    WorkQueue& m_oWorkQueue;
public:
    Worker(WorkQueue& aoWorkQueue) : m_oWorkQueue(aoWorkQueue) {
        
    }

    void work() {
        
    }
};