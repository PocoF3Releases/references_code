#ifndef EVENT_THREAD_INSPECTOR_H_
#define EVENT_THREAD_INSPECTOR_H_

#include <ui/DisplayId.h>
#include <utils/Timers.h>
#include <IMiuiEventThreadInspectorStub.h>

namespace android {

class EventThreadInspector : public IMiuiEventThreadInspectorStub{
public:
    EventThreadInspector(const char* threadName);
    ~EventThreadInspector();
    void checkVsyncWaitTime(nsecs_t waitVsyncStartTime, const char* state,
                            PhysicalDisplayId displayId);

private:
    const char* mThreadName;

    nsecs_t mBadVsyncTotalDuration;
    int mBadVsyncCount;
    nsecs_t mNormalVsyncTotalDuration;
    int mSerialNormalVsyncCount;
    nsecs_t mMaxDelayedVsyncDuration;

    void reportDelayedVsyncs(const char* state, PhysicalDisplayId displayId);
    void reset();
};

} // namespace android

#endif /* EVENT_THREAD_INSPECTOR_H_ */
