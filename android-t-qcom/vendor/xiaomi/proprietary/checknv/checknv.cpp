 /*
  * checknv.c
  *
  * Copyright (C) 2017-2019 Xiaomi Ltd.
  *
  * Author: Chi Zhaolin <chizhaolin@xiaomi.com>
  * 	    Guo Shuo <guoshuo@xiaomi.com>
  * 	    Ge Qi <GeQi@xiaomi.com>
  *
  * All Rights Reserved.
  * Confidential and Proprietary - Xiaomi Ltd.
  *
  * This program is for NV checking, if the NV is destroyed it will reboot to recovery with the bad NV message.
  */


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include <log/log.h>

#include "ModemEventHandler.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "checknv"

static void *uevent_thread(void *)
{
    struct sockaddr_nl nladdr;
    int sz = 64 * 1024;
    int on = 1;
    int sock = -1;

    do {
        memset(&nladdr, 0, sizeof(nladdr));
        nladdr.nl_family = AF_NETLINK;
        nladdr.nl_pid = getpid();
        nladdr.nl_groups = 0xffffffff;

        sock = socket(PF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_KOBJECT_UEVENT);
        if (sock < 0) {
            ALOGE("Unable to create netlink socket");
            break;
        }
        if ((setsockopt(sock, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz)) < 0) &&
            (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz)) < 0)) {
            ALOGE("Unable to set rcvbuf");
            break;
        }

        if (setsockopt(sock, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on)) < 0) {
            ALOGE("Unable set passcred, ignore");
            //return;
        }

        if (bind(sock, (struct sockaddr *)&nladdr, sizeof(nladdr)) < 0) {
            ALOGE("Unable to bind uevent socket");
            break;
        }

        ModemEventHandler *handler = new ModemEventHandler(sock);

        handler->startListener();

        //let the uevent thread to run for 300s, and then terminate.
        ALOGD("Let the uevent thread run for 5mins, 5min is long enough to check if the modem is crashed or not.");
        sleep(300);
        ALOGD("Stop the checknv");

        handler->stopListener();

        delete handler;

    } while(0);

    if (sock >= 0) {
        close(sock);
    }

    return NULL;
}

int main(void)
{
    pthread_t t_runner;
    ALOGI("checknv started");

    if (pthread_create(&t_runner, NULL, uevent_thread, NULL) == -1) {
        ALOGE("Failed to create uevent thread");
        return 0;
    }

    pthread_join(t_runner, NULL);

    ALOGI("checknv end");
    return 0;
}
