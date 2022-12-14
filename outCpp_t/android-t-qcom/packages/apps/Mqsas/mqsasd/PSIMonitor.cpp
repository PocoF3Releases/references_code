#define LOG_TAG "PSITrigger"

#include <string.h>
#include <cutils/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <inttypes.h>

#include "PSIMonitor.h"

#define EPOLL_EVENT_SIZE 10

/* ioctl cmd to get info of  last psi trigger event*/
#define GET_LAST_PSI_EVENT_INFO _IOR('p', 1, struct psi_event_info)

using namespace android;

struct psi_event_info {
    uint64_t last_event_time;
    uint64_t last_event_growth;
};

struct trigger_info {
    int fd;
    int callbackKey;
};

std::vector<PSIMonitor::PsiMonitorConnection*> PSIMonitor::psiMonitorConnections;
std::mutex PSIMonitor::psiMonitorConnectionsLock;
std::unordered_set<int> PSIMonitor::registerTriggerKeys;

volatile static int epfd;
volatile static int socketWriteFd = -1;
volatile static int socketReadFd = -1;

static pthread_t pthread;

void* PSIMonitor::pthread_run(void* args __attribute__((unused))) {
    struct epoll_event events[EPOLL_EVENT_SIZE];
    int nfds = 0;
    struct psi_event_info last_event_info;
    char buffer [128] = {};
    for(;;) {
        nfds = epoll_wait(epfd, events, EPOLL_EVENT_SIZE, -1);
        if (nfds < 0) {
            if (errno == EINTR) {
                sleep(1);
                continue;
            } else {
                return NULL;
            }
        }
        for(int i = 0; i < nfds; i++) {
            struct trigger_info* info = (struct trigger_info*) events[i].data.ptr;
            if (!ioctl(info->fd, GET_LAST_PSI_EVENT_INFO, &last_event_info)) {
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "%d|%" PRIu64 "|%" PRIu64 "", info->callbackKey, last_event_info.last_event_time, last_event_info.last_event_growth);
                if (socketWriteFd != -1) {
                    send(socketWriteFd, buffer, strlen(buffer), 0);
                }
                psiMonitorConnectionsLock.lock();
                for (int i = 0; i < (int)psiMonitorConnections.size(); i++) {
                    PsiMonitorConnection* pConnection = psiMonitorConnections[i];
                    if (pConnection->socketWriteFd >= 0) {
                        send(pConnection->socketWriteFd, buffer, strlen(buffer), 0);
                    }
                }
                psiMonitorConnectionsLock.unlock();
            } else {
                ALOGE("get PSI events detail failed!! errno=%d", errno);
            }
        }
    }
    return NULL;
}

void PSIMonitor::initPSIMonitor() {
    epfd = epoll_create(EPOLL_EVENT_SIZE);
    if (epfd == -1) {
        ALOGE("psi open epoll fd failed!!");
        return;
    }
    int err = pthread_create(&pthread, NULL, pthread_run, NULL);
    if (err != 0) {
        ALOGE("psi monitor thread create failed!!");
        return;
    }

    int sockFds[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sockFds) < 0) {
        ALOGE("open socket pair failed!!");
        return;
    }
    socketWriteFd = sockFds[0];
    socketReadFd = sockFds[1];
}

int PSIMonitor::getPSISocketFd() {
    if (socketReadFd < 0) {
        return -1;
    }
    return dup(socketReadFd);
}

void PSIMonitor::addPSITriggerCommand(int callbackKey, const char* node, const char* command) {
    std::unordered_set<int>::iterator it = registerTriggerKeys.find(callbackKey);
    if (it != registerTriggerKeys.end()) {
        ALOGE("PSI trigger key has been registed!");
        return;
    }

    int nodeFd = open(node, O_RDWR | O_NONBLOCK);
    if (nodeFd < 0) {
        ALOGE("open psi node failed, return code: %d!!", errno);
        return;
    }
    if (write(nodeFd, command, strlen(command) + 1) < 0) {
        ALOGE("write trigger command failed!!");
        return;
    }

    struct epoll_event ev;
    ev.events = EPOLLPRI;
    struct trigger_info* info = (struct trigger_info*) malloc(sizeof(struct trigger_info));
    info->fd = nodeFd;
    info->callbackKey = callbackKey;
    ev.data.ptr = info;
    registerTriggerKeys.insert(callbackKey);

    epoll_ctl(epfd, EPOLL_CTL_ADD, nodeFd, &ev);
}

int PSIMonitor::connectPSIMonitor(const char* clientKey) {
    PsiMonitorConnection* pConnection = nullptr;
    int sockFds[2];

    psiMonitorConnectionsLock.lock();

    for (int i = 0; i < (int)psiMonitorConnections.size(); i++) {
        if (strcmp(clientKey, psiMonitorConnections[i]->clientKey) == 0) {
            pConnection = psiMonitorConnections[i];
            break;
        }
    }

    if (pConnection == nullptr) {
        pConnection = new PsiMonitorConnection();
        pConnection->clientKey = (char*)malloc(strlen(clientKey) + 1);
        strcpy(pConnection->clientKey, clientKey);
        pConnection->socketWriteFd = -1;
        pConnection->socketReadFd = -1;
        psiMonitorConnections.push_back(pConnection);

        if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sockFds) >= 0) {
            pConnection->socketWriteFd = sockFds[0];
            pConnection->socketReadFd = sockFds[1];
        } else {
            ALOGE("open socket pair failed!!");
        }
    }
    psiMonitorConnectionsLock.unlock();

    return pConnection->socketReadFd >= 0 ? dup(pConnection->socketReadFd) : -1;
}
