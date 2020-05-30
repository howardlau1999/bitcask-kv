#include "CurrentThread.h"
#include <unistd.h>
#include <sys/syscall.h>

__thread int CurrentThread::t_cachedTid = 0;
pid_t CurrentThread::getTid() {
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void CurrentThread::cacheTid() {
    if (t_cachedTid == 0) {
        t_cachedTid = CurrentThread::getTid();
    }
}