#ifndef XLOG_H
#define XLOG_H

//xlog version is 20190603

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_LEN 512

struct xlog_msg {
     int msg_id;
     int len;
    char  payload[MAX_LEN];
};


#define XLOG_DEV "/dev/xlog"

#ifdef __cplusplus
}
#endif

#endif