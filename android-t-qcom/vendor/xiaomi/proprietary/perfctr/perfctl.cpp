#include <sys/types.h>
#include <utils/Log.h>
#include <utils/Timers.h>
#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include "perfctl.h"
#include <linux/types.h>
#include <errno.h>
#include <string.h>
#include <utils/Timers.h>
#include <pthread.h>
#include <vector>
#include <string>

#define PERF_LOG_TAG "libperfctl"
#ifdef ALOGD
#undef ALOGD
#define ALOGD(...) do{((void)ALOG(LOG_INFO, PERF_LOG_TAG, __VA_ARGS__));}while(0)
#endif

#define PATH_PERF_IOCTL "/proc/perfmgr/perf_ioctl"


static pthread_mutex_t sMutex = PTHREAD_MUTEX_INITIALIZER;

int devfd = -1;

static inline int check_perf_ioctl_valid(void)
{
    if (devfd >= 0) {
        return 0;
    } else if (devfd == -1) {
        devfd = open(PATH_PERF_IOCTL, O_RDONLY);
        // file not exits
        if (devfd < 0 && errno == ENOENT)
            devfd = -2;
        // file exist, but can't open
        if (devfd == -1) {
            ALOGD("Can't open %s: %s", PATH_PERF_IOCTL, strerror(errno));
            return -1;
        }
    // file not exist
    } else if (devfd == -2) {
        ALOGD("Can't open %s: %s", PATH_PERF_IOCTL, strerror(errno));
        return -2;
    }
    return 0;
}


extern "C"
int xgfNotifyQueue(__u32 value, __u64 bufID, __s32 queue_SF, __u64 identifier)
{
    BUFFER_PACKAGE msg;
    
    pthread_mutex_lock(&sMutex);
    msg.pid = getpid();
    msg.start = value;
    msg.identifier = identifier;
    if (check_perf_ioctl_valid() == 0)
        ioctl(devfd, BUFFER_QUEUE, &msg);

    pthread_mutex_unlock(&sMutex);
    return 0;
}

extern "C"
int xgfNotifyDequeue(__u32 value, __u64 bufID, __s32 queue_SF, __u64 identifier)
{
    BUFFER_PACKAGE msg;

    pthread_mutex_lock(&sMutex);
    msg.pid = getpid();
    msg.start = value;
    msg.identifier = identifier;
    if (check_perf_ioctl_valid() == 0)
        ioctl(devfd, BUFFER_DEQUEUE, &msg);

    pthread_mutex_unlock(&sMutex);
    return 0;
}

extern "C"
int xgfNotifyConnect(__u32 value, __u64 bufID, __u32 connectedAPI, __u64 identifier)
{
    BUFFER_PACKAGE msg;

    msg.pid = getpid();
    msg.identifier = identifier;
    if (value)
        msg.connectedAPI = connectedAPI;
    else
        msg.connectedAPI = 0;
    if (check_perf_ioctl_valid() == 0)
        ioctl(devfd, BUFFER_QUEUE_CONNECT, &msg);

    return 0;
}


extern "C"
int xgfNotifyVsync(__u32 value)
{
    BUFFER_PACKAGE msg;
 
    msg.pid = getpid();
    msg.frame_time = value;
    if (check_perf_ioctl_valid() == 0)
        ioctl(devfd, BUFFER_VSYNC, &msg);

    return 0;
}



