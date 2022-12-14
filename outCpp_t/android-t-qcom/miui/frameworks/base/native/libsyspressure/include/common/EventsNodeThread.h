#pragma once

#include <vector>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/Thread.h>
#include "common/Common.h"

namespace android {

namespace syspressure {

class EventsNodeThread : public Thread {
public:
    EventsNodeThread(std::string path);
    EventsNodeThread(Vector<std::string> paths);
    ~EventsNodeThread();

    virtual void requestExit();
    virtual status_t requestExitAndWait();

    using EventsDataCall = std::function<void(const std::string path, const std::string buffer)>;
    void setEventsDataCall(EventsDataCall callback);
private:
    virtual status_t readyToRun();
    virtual bool threadLoop();
    struct WatchInfo {
        std::string path;
        int watchID;
    };
    bool addWatch(std::string path);
    bool removeWatch(std::string path);
    WatchInfo* getWatchInfo(std::string path);
    std::string getNodePath(int wd) const;
    std::string readFile(const std::string& filePath) const;
private:
    int mInotifyFd;
    Vector<std::string> mEventsPath;
    Vector<WatchInfo> mWatchInfos;
    EventsDataCall mEventsDataCall;
    // variables above are unguarded:
    // -- accessed in thread loop or in constructor only
    Mutex mMutex;
    bool mRunning;          // guarding only when it's important
};

} // namespace syspressure

} // namespace android
