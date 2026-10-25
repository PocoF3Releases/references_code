#pragma once

#include <string>
#include <utils/threads.h>

namespace android {

#define TOMBSTONE_MONITOR_DIR_PROP    "sys.tombstone.monitor.in.dir"
#define TOMBSTONE_COPY_TO_DIR_PROP    "sys.tombstone.monitor.out.dir"
#define TOMBSTONE_MONITOR_DIR         "/data/tombstones/"
#define TOMBSTONE_COPY_TO_DIR         "/data/miuilog/stability/nativecrash/tombstones/"

    class TombstoneMonitorThread : public Thread {
    private:
        inline bool isFile(const std::string& path);
        bool copy(const std::string& src, const std::string& dest);

    protected:
        virtual bool threadLoop();
    };
}