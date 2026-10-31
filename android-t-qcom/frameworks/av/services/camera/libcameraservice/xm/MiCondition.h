#ifndef ANDROID_SERVERS_MI_CONDITION_H
#define ANDROID_SERVERS_MI_CONDITION_H
#include <limits.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
# include <pthread.h>
#include <utils/Errors.h>
#include <utils/Timers.h>
// ---------------------------------------------------------------------------
namespace android {
// ---------------------------------------------------------------------------
// DO NOT USE: please use std::condition_variable instead.
/*
 * Condition variable class.  The implementation is system-dependent.
 *
 * Condition variables are paired up with mutexes.  Lock the mutex,
 * call wait(), then either re-wait() if things aren't quite what you want,
 * or unlock the mutex and continue.  All threads calling wait() must
 * use the same mutex for a given Condition.
 *
 * On Android and Apple platforms, these are implemented as a simple wrapper
 * around pthread condition variables.  Care must be taken to abide by
 * the pthreads semantics, in particular, a boolean predicate must
 * be re-evaluated after a wake-up, as spurious wake-ups may happen.
 */
class MiCondition {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };
    enum WakeUpType {
        WAKE_UP_ONE = 0,
        WAKE_UP_ALL = 1
    };
    MiCondition();
    explicit MiCondition(int type);
    ~MiCondition();
    // Wait on the condition variable.  Lock the mutex before calling.
    // Note that spurious wake-ups may happen.
    status_t wait(std::mutex& mutex);
    // same with relative timeout
    status_t waitRelative(std::mutex& mutex, nsecs_t reltime);
    // Signal the condition variable, allowing one thread to continue.
    void signal();
    // Signal the condition variable, allowing one or all threads to continue.
    void signal(WakeUpType type) {
        if (type == WAKE_UP_ONE) {
            signal();
        } else {
            broadcast();
        }
    }
    // Signal the condition variable, allowing all threads to continue.
    void broadcast();
private:
    pthread_cond_t mCond;
};
// ---------------------------------------------------------------------------
inline MiCondition::MiCondition() : MiCondition(PRIVATE) {
}
inline MiCondition::MiCondition(int type) {
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
#if defined(__linux__)
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
#endif
    if (type == SHARED) {
        pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    }
    pthread_cond_init(&mCond, &attr);
    pthread_condattr_destroy(&attr);
}
inline MiCondition::~MiCondition() {
    pthread_cond_destroy(&mCond);
}
inline status_t MiCondition::wait(std::mutex& mutex) {
    return -pthread_cond_wait(&mCond, mutex.native_handle());
}
inline status_t MiCondition::waitRelative(std::mutex& mutex, nsecs_t reltime) {
    struct timespec ts;
#if defined(__linux__)
    clock_gettime(CLOCK_MONOTONIC, &ts);
#else // __APPLE__
    // Apple doesn't support POSIX clocks.
    struct timeval t;
    gettimeofday(&t, nullptr);
    ts.tv_sec = t.tv_sec;
    ts.tv_nsec = t.tv_usec*1000;
#endif
    // On 32-bit devices, tv_sec is 32-bit, but `reltime` is 64-bit.
    int64_t reltime_sec = reltime/1000000000;
    ts.tv_nsec += static_cast<long>(reltime%1000000000);
    if (reltime_sec < INT64_MAX && ts.tv_nsec >= 1000000000) {
        ts.tv_nsec -= 1000000000;
        ++reltime_sec;
    }
    int64_t time_sec = ts.tv_sec;
    if (time_sec > INT64_MAX - reltime_sec) {
        time_sec = INT64_MAX;
    } else {
        time_sec += reltime_sec;
    }
    ts.tv_sec = (time_sec > LONG_MAX) ? LONG_MAX : static_cast<long>(time_sec);
    return -pthread_cond_timedwait(&mCond, mutex.native_handle(), &ts);
}
inline void MiCondition::signal() {
    pthread_cond_signal(&mCond);
}
inline void MiCondition::broadcast() {
    pthread_cond_broadcast(&mCond);
}
// ---------------------------------------------------------------------------
}  // namespace android
// ---------------------------------------------------------------------------
#endif // ANDROID_SERVERS_MI_CONDITION_H

