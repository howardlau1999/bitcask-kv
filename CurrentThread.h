#pragma once
#include <sys/types.h>

namespace CurrentThread {
    extern __thread int t_cachedTid;
    void cacheTid();
    pid_t getTid();


    inline int tid() {
        if (__builtin_expect(t_cachedTid == 0, 0)) {
            cacheTid();
        }
        return t_cachedTid;
    }
};