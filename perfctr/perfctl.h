#ifndef PERFIOCTL_H
#define PERFIOCTL_H

#include <linux/types.h>
#include <linux/ioctl.h>

#include <vector>

using namespace std;


typedef struct _BUFFER_PACKAGE {
    __u32 pid;
    union {
        __u32 start;
        __u32 connectedAPI;
    };
    union {
        __u64 frame_time;
        __u64 bufID;
    };
    __u64 frame_id; 
    __s32 queue_SF;
    __u64 identifier;
} BUFFER_PACKAGE;


#define BUFFER_QUEUE                  _IOW('g', 1, BUFFER_PACKAGE)
#define BUFFER_DEQUEUE                _IOW('g', 3, BUFFER_PACKAGE)
#define BUFFER_VSYNC                  _IOW('g', 5, BUFFER_PACKAGE)
#define BUFFER_TOUCH                  _IOW('g', 10, BUFFER_PACKAGE)
#define BUFFER_QUEUE_CONNECT          _IOW('g', 15, BUFFER_PACKAGE)


#endif
