#ifndef __TOUCHSOCKET_H__
#define __TOUCHSOCKET_H__

#include <utils/threads.h>

#define SOCKET_NAME "touchevent"

using namespace android;

enum {
    CMD_GET_TOUCHEVENT = 1,
    CMD_REPLY = 100,
};

struct  msg_t {
    int cmd;
    int length;
    char message[1024];
};

class TouchServer : public android::Thread {
public:
    TouchServer() {}
    ~TouchServer();
private:
    virtual bool threadLoop();
    int createSocket(const char *name);
    void handleMessage(int fd, msg_t* msg);

    int server_fd;
};

#endif
