/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin sample1 head file
 *
 */

#ifndef MISIGHT_PLUGIN_SAMPLE_EVENT_SOURCE
#define MISIGHT_PLUGIN_SAMPLE_EVENT_SOURCE

#include <string>

#include "event_loop.h"
#include "event_source.h"


namespace android {
namespace MiSight {
class EventSourceSample : public FileEventHandler, public EventSource {
public:
    EventSourceSample() : eventId_(0),start_(false){};
    ~EventSourceSample() {
         printf("~EventSourceSample, exit xxxxxxxxxxxxxxxxxxxxxxxxx\n");
    };

    void OnLoad() override;
    void OnUnload() override;
    void StartEventSource() override;


    int32_t OnFileEvent(int fd, int Type) override;
    int32_t GetPollFd() override;
    int32_t GetPollEventType() override;
    bool IsStart() {
        return start_;
    }
    void CreateTestFile(int index, int type = 0);
    void StopMonitor(int type);

private:
    void PublishEvent(const std::string& file);
    const std::string WATCH_LOG_PATH = "/data/test/eventsource";
    const std::string WATCH_ERR_PATH = "/data/test/eventerr";
    int inotifyFd_[3];
    int eventId_;
    bool start_;
    std::map<std::string, int> fileMap_;
};


}
}
#endif
