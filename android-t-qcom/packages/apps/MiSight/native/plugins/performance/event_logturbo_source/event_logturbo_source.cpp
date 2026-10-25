/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event radiator plugin implement file
 *
 */

#include "event_logturbo_source.h"

#include <fcntl.h>
#include <list>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <regex>
#include "event_util.h"
#include "log.h"
#include "plugin_factory.h"
#include "string_util.h"
#include "logturbo_event_utils.h"

namespace android {
namespace MiSight {

REGISTER_PLUGIN(EventLogturboSource);

void EventLogturboSource::OnLoad()
{
    // set plugin info
    MISIGHT_LOGD("Load logturbo source ");
    SetVersion("EventLogturboSource-1.0");
    SetName("EventLogturboSource");
}

void EventLogturboSource::OnUnload()
{
    exit_ = true;
    if (mFileMonitor != nullptr) {
    mFileMonitor->unregisterExceptionLogMonitor(mEpollFd, mExceptionLogFd);
    mFileMonitor->destoryExceptionLogFd(mExceptionLogFd);
    delete mFileMonitor;
    mFileMonitor = nullptr;
    }
}
void EventLogturboSource::StartEventSource() {
    MISIGHT_LOGD("Logturbo start init");
    mFileMonitor = new FileMonitor();
    mExceptionLogFd = mFileMonitor->openExceptionLogFd();
    if (mExceptionLogFd != -1) {
        mEpollFd = mFileMonitor->registerExceptionLogMonitor(mExceptionLogFd);
        if (mEpollFd != -1) {
            threadLoop();
        }
    }
}

bool EventLogturboSource::threadLoop() {
    MISIGHT_LOGD("LogTurbo start loop");
    do {
        struct epoll_event events[MAX_EPOLL_EVENTS];
        int nevents = epoll_wait(mEpollFd, events, MAX_EPOLL_EVENTS, -1);

        if (nevents == -1) {
            if (errno == EINTR) {
                continue;
            }
            MISIGHT_LOGE("epoll_wait: %s", strerror(errno));
            continue;
        }

        char buf[1024] = {};
        ssize_t cnt = read(mExceptionLogFd, buf, sizeof(buf)-1);
        if (cnt <= 0) {
            continue;
        }
        //MISIGHT_LOGD("%zu, rev str %s", strlen(buf), buf); //For debug
        ParseEvent(std::string(buf));
        memset(buf, '\0', sizeof(buf));
    } while(!exit_);
    return false;
}

void EventLogturboSource::ParseEvent(const std::string& eventStr)
{
    // eventStr:
    // EventId 90xxxxxxxxx -t Timestamp -paraList jsonstr
    sp<Event> event = new Event("EventLogturboSource");
    if (event == nullptr) {
        MISIGHT_LOGE("Event Build Failed");
        return;
    }
    event->eventId_ =  PERF_EVENT_ID_LOGTURBO_SOURCE;
    event->SetValue(PERF_DEV_SAVE_EVENT, eventStr);
    PublishPipelineEvent(event);
}

}
}

