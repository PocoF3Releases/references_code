#include <unistd.h>
#include <errno.h>
#include <list>
#include <string>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <cutils/klog.h>
#include <unordered_map>
#include <pthread.h>
#include <semaphore.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "TouchServer.h"
#include "TouchMain.h"

#define MAX_EVENTS 16

TouchServer::~TouchServer() {
    close(server_fd);
}

bool TouchServer::threadLoop() {
    int ret = 0, client_fd, epfd;
    struct sockaddr_un addr;
    socklen_t addr_size = sizeof(addr);
    struct epoll_event event, events[MAX_EVENTS];
    msg_t msg;

    server_fd = createSocket(SOCKET_NAME);

    if(server_fd < 0) {
        return false;
    }
    listen(server_fd, 8);

    epfd = epoll_create(8);
    event.data.fd = server_fd;
    event.events = EPOLLIN;

    epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &event);

    memset(&msg, 0, sizeof(msg_t));
    while (1) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd) {
                if ((client_fd = accept(server_fd, (struct sockaddr *) &addr, &addr_size)) < 0) {
                    ALOGE("Failed to accept, error=%s\n", strerror(errno));
                    continue;
                }
                ALOGI("Success to accept, fd=%d\n", client_fd);
                event.data.fd = client_fd;
                event.events = EPOLLIN | EPOLLHUP;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
                    ALOGE("Faile to epoll_ctl EPOLL_CTL_ADD fd=%d\n", client_fd);
                }
            } else {
                if (events[i].events & EPOLLHUP) {
                    int client_fd = events[i].data.fd;
                    if (epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL) < 0) {
                        ALOGE("Faile to epoll_ctl EPOLL_CTL_DEL fd=%d\n", client_fd);
                    }
                    ALOGI("peer socket closed\n");
                    close(client_fd);
                } else if (events[i].events & EPOLLIN) {
                    int client_fd = events[i].data.fd;
                    ret = recv(client_fd, &msg, sizeof(msg), 0);
                    if (ret < 0) {
                        ALOGE("Failed to recv, fd=%d, error=%s\n", client_fd, strerror(errno));
                        continue;
                    }
                    handleMessage(client_fd, &msg);
                }
            }
        }
    }
    close(epfd);
    return true;
}

int TouchServer::createSocket(const char *name) {
    struct sockaddr_un addr;
    size_t addr_len;
    int sockfd, ret;

    if (server_fd > 0) {
        return server_fd;
    }
    if (name == NULL) {
        ALOGE("NULL params\n");
        return -1;
    }
    sockfd = socket(PF_UNIX, SOCK_SEQPACKET, 0);
    if (sockfd < 0) {
        ALOGE("Failed to open socket '%s', error=%s\n", name, strerror(errno));
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path + 1, name, strlen(name));
    addr_len = offsetof(struct sockaddr_un, sun_path) + strlen(name) + 1;

    ret = ::bind(sockfd, (struct sockaddr *) &addr, addr_len);
    if (ret) {
        ALOGE("Failed to bind socket '%s', error=%s\n", name, strerror(errno));
        close(sockfd);
        return -1;
    }

    ALOGI("Created socket success with '%s', sockfd=%d\n", name, sockfd);

    return sockfd;
}

void TouchServer::handleMessage(int fd, msg_t *msg) {
    ALOGI("recv fd=%d cmd=%d, length=%d\n", fd, msg->cmd, msg->length);
    switch(msg->cmd) {
    case CMD_GET_TOUCHEVENT: {
        const char* str = get_event_string();
        msg->cmd = CMD_REPLY;
        msg->length = 0;
        if (str) {
            msg->length = strlen(str);
            strncpy(msg->message, str, msg->length);
        }
        ALOGI("send cmd=%d, length=%d", msg->cmd, msg->length);
        int ret = send(fd, msg, sizeof(msg_t), 0);
        if (ret < 0) {
            ALOGI("Failed to send, fd=%d, error=%s\n", fd, strerror(errno));
        }
        break;
    }
    default:
        ALOGW("Invalid cmd %d\n", msg->cmd);
        break;
    }
}
