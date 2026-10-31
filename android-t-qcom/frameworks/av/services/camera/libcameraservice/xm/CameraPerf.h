#ifdef __XIAOMI_CAMERA_PERF__
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <cutils/properties.h>

//#define CAMERAPERF_ENABLE_TRACE 1
#if CAMERAPERF_ENABLE_TRACE
#include <cutils/trace.h>

#ifdef ATRACE_TAG
#undef ATRACE_TAG
#endif
#define ATRACE_TAG ATRACE_TAG_CAMERA
#define CAMERAPERF_TRACE_BEGIN(tid, oldPriority, newPriority) \
            char traceTag[64]; \
            snprintf(traceTag, sizeof(traceTag), "Thread %d priority %d -> %d", \
            tid ? tid : syscall(__NR_gettid), oldPriority, newPriority); \
            ATRACE_BEGIN(traceTag);
#define CAMERAPERF_TRACE_END ATRACE_END
#else
#define CAMERAPERF_TRACE_BEGIN(tid, oldPriority, newPriority)
#define CAMERAPERF_TRACE_END()
#endif

#define GET_PRIORITY \
int newPriority = -10; //UX level
//int newPriority = property_get_int32("camera.thread.ni", -10);  // get NI value from property

#define GET_HIGHER_PRIORITY \
int newHigherPriority = -19;

#define TEMP_THREAD_HIGHER_PRIORITY() \
GET_HIGHER_PRIORITY \
::android::CameraPerf::TemporaryThreadPriority ttp##__LINE__(0, newHigherPriority, true)

#define TEMP_THREAD_PRIORITY() \
GET_PRIORITY \
::android::CameraPerf::TemporaryThreadPriority ttp##__LINE__(0, newPriority, true)

#define CONDITION_THREAD_PRIORITY(condition) \
GET_PRIORITY \
::android::CameraPerf::TemporaryThreadPriority ttp##__LINE__(0, newPriority, condition)

namespace android {

class CameraPerf {

public:
    class TemporaryThreadPriority {
        public:
            // For special thread and need condition is true. If tid is 0, it means current thread.
            // Please note that this class only supports to increase thread priority, if the new priority
            // is lower than the old one, it will do nothing.
            inline TemporaryThreadPriority(int tid, int priority, bool condition) :
                mTid(tid),
                mNewPriority(priority),
                mCondition(condition)
            {
                if (mCondition) {
                    mOldPriority = GetThreadPriority(mTid);
                    if (mOldPriority > mNewPriority) {
                        SetThreadPriority(mTid, mNewPriority);
                        CAMERAPERF_TRACE_BEGIN(mTid, mOldPriority, mNewPriority);
                    }
                }
            }

            inline ~TemporaryThreadPriority() {
                if (mCondition) {
                    if (mOldPriority > mNewPriority) {
                        SetThreadPriority(mTid, mOldPriority);  // restore
                        CAMERAPERF_TRACE_END();
                    }
                }
            }
        private:
            int mTid;
            int mNewPriority;
            int mOldPriority;
            bool mCondition;
    };

    static int GetThreadPriority() {
        return GetThreadPriority(0);
    }

    static int SetThreadPriority(int newPriority) {
        return SetThreadPriority(0, newPriority);
    }

    static int GetThreadPriority(int tid) {
        int ret = getpriority(PRIO_PROCESS, tid);
        if (ret == -1 && errno) {
            ALOGE("CameraPerf: getpriority fail, tid is %d, errno is %d",
                  tid ? tid : (int)syscall(__NR_gettid), errno);
        } else {
            ALOGD("CameraPerf: getpriority success, tid is %d, priority is %d",
                  tid ? tid : (int)syscall(__NR_gettid), ret);
        }
        return ret;
    }

    static int SetThreadPriority(int tid, int newPriority) {
        int ret = 0;
        if (newPriority != getpriority(PRIO_PROCESS, tid)) {
            ret = setpriority(PRIO_PROCESS, tid, newPriority);
            if (ret == -1) {
                ALOGE("CameraPerf: setpriority fail, tid is %d, priority is %d, ret is %d, errno is %d",
                      tid  ? tid : (int)syscall(__NR_gettid), newPriority, ret, errno);
            } else {
                ALOGD("CameraPerf: setpriority success, tid is %d, priority is %d",
                      tid  ? tid : (int)syscall(__NR_gettid), newPriority /*getpriority(PRIO_PROCESS, tid)*/);
            }
        }
        return ret;
    }
};
}

#endif
