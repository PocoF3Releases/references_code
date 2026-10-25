/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event radiator plugin implement file
 *
 */

#include "event_radiator.h"

#include <fcntl.h>
#include <inttypes.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "event_util.h"
#include "log.h"
#include "plugin_factory.h"
#include "string_util.h"


namespace android {
namespace MiSight {

REGISTER_PLUGIN(EventRadiator);

const std::string LOG_EXCEPTION = "/dev/miev";
constexpr int32_t READ_BUF_SIZE = 4 * 1024 +1;

void EventRadiator::OnLoad()
{
    // set plugin info
    SetVersion("EventRadiator-1.0");
    SetName("EventRadiator");
}

void EventRadiator::OnUnload()
{
    exit_ = true;
}

void EventRadiator::StartEventSource()
{
    int fd = open(LOG_EXCEPTION.c_str(), O_RDWR);
    if (fd <= 0) {
        MISIGHT_LOGE("open log exception failed");
        return;
    }

    struct timeval tv;
    char buf[READ_BUF_SIZE] = {'\0'};
    for (;;) {
        if (exit_) {
            break;
        }

        tv.tv_sec = waitTime_;
        tv.tv_usec = 0;

        fd_set rdFds;
        FD_ZERO(&rdFds);
        FD_SET(fd, &rdFds);

        int ret = select(fd +1, &rdFds, nullptr, nullptr, nullptr);
        if (ret <= 0) {
            continue;
        }
        if (!FD_ISSET(fd, &rdFds)) {
            continue;
        }

        while (read(fd, buf, READ_BUF_SIZE -1) > 0) {
            ParseEvent(buf);
            memset(buf, '\0', READ_BUF_SIZE);
        }
    }

    close(fd);
}

void EventRadiator::ParseEvent(char* buf)
{
    // EventId 90xxxxxxxxx -t Timestamp -paraList jsonstr
    int eventId = 0;
    uint64_t happenTime = 0;
    char jsonStr[READ_BUF_SIZE] = {'\0'};
    int ret = sscanf(buf, "EventId %d -t %" PRIu64 " -paraList %s", &eventId, &happenTime, jsonStr);
    if (ret < 3) {
        MISIGHT_LOGI("event invalid");
        return;
    }
    sp<Event> event = new Event("EventRadiator");
    if (event == nullptr) {
        return;
    }
    event->eventId_ = eventId;
    event->happenTime_ = happenTime;
    event->SetValue(EventUtil::DEV_SAVE_EVENT, std::string(jsonStr));
    PublishPipelineEvent(event);
}
}
}

