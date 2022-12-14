/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin sample1 implement file
 *
 */

#include "event_source_sample.h"

#include <fstream>
#include <iostream>
#include <string>


#include <sys/stat.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/inotify.h>

#include "common.h"
#include "event_loop.h"
#include "plugin_factory.h"

namespace android {
namespace MiSight {
REGISTER_PLUGIN(EventSourceSample);
void EventSourceSample::OnLoad()
{
    printf("EventSourceSample::OnLoad\n");
    SetName("EventSourceSample");

    int isCreate = ::mkdir((WATCH_LOG_PATH + "0").c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
    if (!isCreate) {
        printf("create path:%s 0\n", WATCH_LOG_PATH.c_str());
    }

    isCreate = ::mkdir((WATCH_LOG_PATH + "1").c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
    if (!isCreate) {
        printf("create path:%s 1\n", WATCH_LOG_PATH.c_str());
    }

    isCreate = ::mkdir((WATCH_LOG_PATH + "2").c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
    if (!isCreate) {
        printf("create path:%s 2\n", WATCH_LOG_PATH.c_str());
    }
    start_ = true;
}

void EventSourceSample::CreateTestFile(int eventId, int type)
{
    printf("EventSourceSample:create Testfile\n");
    std::string path = WATCH_LOG_PATH + std::to_string(type) + "/test_"+ std::to_string(eventId);
    std::ofstream file(path);
    if (!file.good()) {
        printf("Fail to create watch file:%s\n", path.c_str());
        return;
    }
    file << "cdddddd" << std::endl;
    file.close();
    eventId_ = eventId;
}

void EventSourceSample::StopMonitor(int type)
{
    if (type > 2) {
        return;
    }

    if (inotifyFd_[type] < 0) {
        return;
    }
    GetLooper()->RemoveFileEventHandler("monitorFile" + std::to_string(type));

    inotifyFd_[type] = -1;
}

void EventSourceSample::OnUnload()
{
    printf("EventSourceSample::OnUnload\n");
}

void EventSourceSample::StartEventSource()
{
    printf("EventSourceSample::StartEventSource\n");

    eventId_ = 0;
    GetLooper()->AddFileEventHandler("monitorFile0", this);
    printf("EventSourceSample::StartEventSource after add file 0 event handler\n");

    eventId_ = 1;
    GetLooper()->AddFileEventHandler("monitorFile1", this);
    printf("EventSourceSample::StartEventSource after add file 1 event handler\n");

    eventId_ = 2;
    GetLooper()->AddFileEventHandler("monitorFile2", this);
    printf("EventSourceSample::StartEventSource after add file 2 event handler\n");

}

int32_t EventSourceSample::OnFileEvent(int fd, int type)
{
    printf("EventSourceSample::OnEvent fd:%d, type:%d, inotifyFd_:%d,%d,%d\n", fd, type, inotifyFd_[0], inotifyFd_[1], inotifyFd_[2]);
    const int bufSize = 2048;
    char buffer[bufSize] = {0};
    char *offset = nullptr;
    struct inotify_event *event = nullptr;

    int len = read(fd, buffer, bufSize);
    if (len < 0) {
        printf("EventSourceSample failed to read event");
        return 1;
    }

    offset = buffer;
    event = (struct inotify_event *)buffer;
    while ((reinterpret_cast<char *>(event) - buffer) < len) {
        for (const auto &it : fileMap_) {
            if (it.second != event->wd) {
                continue;
            }

            if (event->name[event->len - 1] != '\0') {
                event->name[event->len - 1] = '\0';
            }
            std::string filePath = it.first + "/" + std::string(event->name);
            printf("handle file event in %s\n", filePath.c_str());
            PublishEvent(filePath);
        }
        int tmpLen = sizeof(struct inotify_event) + event->len;
        event = (struct inotify_event *)(offset + tmpLen);
        offset += tmpLen;
    }
    if (fd == inotifyFd_[2]) {
        return 0;
    }
    return 1;
}

int32_t EventSourceSample::GetPollFd()
{
    printf("EventSourceSample::GetPollFd %d\n", eventId_);
    if (eventId_ > 2) {
        return -1;
    }
    if (inotifyFd_[eventId_] > 0) {
        return inotifyFd_[eventId_ -1];
    }

    inotifyFd_[eventId_] = inotify_init();
    if (inotifyFd_[eventId_] == -1) {
        printf("EventSourceSample failed to init inotify: %s\n", strerror(errno));
        return -1;
    }

    int wd = inotify_add_watch(inotifyFd_[eventId_], (WATCH_LOG_PATH + std::to_string(eventId_)).c_str(), IN_CLOSE_WRITE | IN_MOVED_TO);
    if (wd < 0) {
        printf("EventSourceSample failed to add watch entry : %s %s\n", strerror(errno), (WATCH_LOG_PATH + std::to_string(eventId_)).c_str());
        close(inotifyFd_[eventId_]);
        inotifyFd_[eventId_] = -1;
        return -1;
    }

    printf("EventSourceSample GetPollFd %d %d\n", inotifyFd_[eventId_], wd);
    fileMap_[WATCH_LOG_PATH] = wd;
    return inotifyFd_[eventId_];
}

int32_t EventSourceSample::GetPollEventType()
{
    printf("EventSourceSample::GetPollType\n");
    return EPOLLIN;
}

void EventSourceSample::PublishEvent(const std::string &file)
{
    // create a pipeline event
    sp<Event> event = new Event("EventSourceSample");
    event->SetValue("file", file);
    // add special information
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    event->eventId_ = eventId_;
    event->SetValue(std::to_string(eventId_), "EventSourceSample");
    PublishPipelineEvent(event);
}
}
}
