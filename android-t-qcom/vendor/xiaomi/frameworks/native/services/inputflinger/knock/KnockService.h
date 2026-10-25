
#ifndef _UI_KNOCK_SERVICE_H
#define _UI_KNOCK_SERVICE_H

#include <android-base/file.h>
#include <android-base/strings.h>
#include <cutils/properties.h>
#include <errno.h>
#include <input/DisplayViewport.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utils/Looper.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <utils/Timers.h>
#include <utils/threads.h>
#include <vector>
#include "InputListener.h"
//#include <utils/String8.h>
#include <math.h>
#include <sensor/Sensor.h>
#include <sensor/SensorManager.h>
#include "InputThread.h"
#include "cutils/properties.h"

//#ifdef KNOCK_SUPPORT
#include <android/binder_manager.h>
#include "aidl/xiaomi/knock/algorithm/BnKnockAlgorithm.h"
//#endif

#include "InputKnockBase.h"

using ::aidl::xiaomi::knock::algorithm::IKnockAlgorithm;
using ndk::SpAIBinder;
namespace android {

class KnockEpollThread;

class KnockService : public KnockServiceInterface {
public:
    enum {
        AMOTION_EVENT_TOOL_TYPE_KNOCK = 32,
    };

    struct SensorEntry {
        nsecs_t time;
        int data;

    public:
        SensorEntry(nsecs_t time, int data);

        ~SensorEntry();
    };

    enum {
        TYPE_START_READ_DIFF,
        TYPE_MODIFY_TOOL_TYPE,
        TYPE_ACTION_DOWN_DELAY,
        TYPE_KNOCK_REPEAT_CHECK,
        TYPE_INIT_SNPE,
    };

    enum {
        KNOCK_SERVICE_STATE_RECEIVING,
        KNOCK_SERVICE_STATE_PROCESSING,
        KNOCK_SERVICE_STATE_KNOCK_REPEAT_CHECKING,
        KNOCK_SERVICE_STATE_FINISH,
    };

    enum {
        KNOCK_ALGORITHM_RESULT_KNOCK,
        KNOCK_ALGORITHM_RESULT_TOUCH,
    };

    enum {
        //未开始收集，第一帧验证失败
        RAW_DATA_COLLECT_PENDING,
        //开始收集，第一帧验证正常
        RAW_DATA_COLLECT_RECEIVING,
        //收集成功，已满足指定帧数
        RAW_DATA_COLLECT_SUCCESS,
        //收集失败，有坏数据，未满足指定帧数
        RAW_DATA_COLLECT_FAIL,
    };

    KnockService(InputListenerInterface* listener);

    virtual ~KnockService();

    status_t start() override;

    status_t stop() override;

    void dump(std::string& dump) override;

    bool isOpenNativeKnock() override;

    bool isNeedSaveMotion(NotifyMotionArgs* args, int surfaceOrientation) override;

    bool isIgnoreSensor();

    void notifyAccelerometeSensorChange(float data);

    virtual void notifyKeyDataReady(bool keyDown) override;

    virtual void loopOnce();

    virtual void notifyDeviceTxRx(int tx, int rx);

    virtual void notifyRawTouch(int* data);

    virtual int getMaxInputFrame();

private:
    template <typename T>
    struct Link {
        T* next;
        T* prev;

    protected:
        inline Link() : next(nullptr), prev(nullptr) {}
    };

    struct CommandEntry : Link<CommandEntry> {
        NotifyMotionArgs* mMotionArgs;
        int32_t type;

    public:
        CommandEntry(NotifyMotionArgs* args, int32_t type);

        virtual ~CommandEntry();
    };

    // Generic queue implementation.
    template <typename T>
    struct Queue {
        T* head;
        T* tail;
        uint32_t entryCount;

        inline Queue() : head(nullptr), tail(nullptr), entryCount(0) {}

        inline bool isEmpty() const { return !head; }

        inline void enqueueAtTail(T* entry) {
            entryCount++;
            entry->prev = tail;
            if (tail) {
                tail->next = entry;
            } else {
                head = entry;
            }
            entry->next = nullptr;
            tail = entry;
        }

        inline void enqueueAtHead(T* entry) {
            entryCount++;
            entry->next = head;
            if (head) {
                head->prev = entry;
            } else {
                tail = entry;
            }
            entry->prev = nullptr;
            head = entry;
        }

        inline void dequeue(T* entry) {
            entryCount--;
            if (entry->prev) {
                entry->prev->next = entry->next;
            } else {
                head = entry->next;
            }
            if (entry->next) {
                entry->next->prev = entry->prev;
            } else {
                tail = entry->prev;
            }
        }

        inline T* dequeueAtHead() {
            entryCount--;
            T* entry = head;
            head = entry->next;
            if (head) {
                head->prev = nullptr;
            } else {
                tail = nullptr;
            }
            return entry;
        }

        uint32_t count() const { return entryCount; }
    };

    typedef unsigned char u8;

    struct DiffPartData {
        u8 flag;
        u8 x;
        u8 y;
        u8 frame;
        short int data[49];
    };
    const char* DIFFDATA_PART_PATH = "/sys/class/touch/touch_dev/partial_diff_data";

    struct DownInfo {
        nsecs_t downTime;
        nsecs_t upTime;
        float downX;
        float downY;
        bool isMultipleTouch;
        int surfaceOrientation;
        bool isFastSlide;
        bool isFastSlideChecking;
    };

    struct KnockRegion {
        int knockMaxValidLeft;
        int knockMaxValidTop;
        int knockMaxValidRight;
        int knockMaxValidBottom;

        int knockInValidLeft;
        int knockInValidTop;
        int knockInValidRight;
        int knockInValidBottom;

        void reset() {
            knockMaxValidLeft = -1;
            knockMaxValidTop = -1;
            knockMaxValidRight = -1;
            knockMaxValidBottom = -1;
            knockInValidLeft = -1;
            knockInValidTop = -1;
            knockInValidRight = -1;
            knockInValidBottom = -1;
        }

        bool isInit() {
            if (knockMaxValidLeft == -1 || knockMaxValidTop == -1 || knockMaxValidRight == -1 ||
                knockMaxValidBottom == -1) {
                return false;
            }
            return true;
        }

        bool isInValidRegion(float x, float y) {
            if (x > knockInValidLeft && x < knockInValidRight && y > knockInValidTop &&
                y < knockInValidBottom) {
                return false;
            }
            if ((x > knockMaxValidLeft && x < knockMaxValidRight && y > knockMaxValidTop &&
                 y < knockMaxValidBottom)) {
                return true;
            }
            return false;
        }
    } mKnockRegion;

    struct DiffDataInfo {
        int centerX;
        int centerY;
        int collectState;
        float diffdataArray[500];
    };

    struct KnockState {
        bool collectData;
        int32_t saveFrameCount;
        nsecs_t waitUseFrameTime;
    } mKnockState;

    const char* DEFAULT_DLC_PATH = "/vendor/etc/knock.dlc";
    const char* PROPERTY_DISPLAY_VERSION = "vendor.panel.vendor";
    const int KNOCK_FEATRUE_OPEN = 1 << 0;
    const float TWICE_DOWN_NEAR_DISTANCE = 300.0f;
    std::unique_ptr<InputThread> mThread;
    char* mDlcPath = new char[35];
    Mutex mLock;
    sp<Looper> mLooper;
    int mKnockServiceState GUARDED_BY(mLock);
    int mCurrentToolTypeResult GUARDED_BY(mLock);
    Queue<CommandEntry> mCommandQueue GUARDED_BY(mLock);
    InputListenerInterface* mMiuiInputMapper;
    std::shared_ptr<IKnockAlgorithm> mKnockAlgorithm;

    char mDisplayVersion[12] = "none";
    bool mKnockFeatureOpen = false;
    bool mKnockSnpeDlcInitResult = false;
    bool mSnpeNotInit = true;
    float mKnockScoreThreshold = 0.6f;
    std::vector<int> mSensorThresholdArray = {60, 60, 60, 60};
    int mKnockUseFrame;
    float mKnockQuickMoveSpeed = 2.5f;
    DownInfo mLastDownInfo GUARDED_BY(mLock);
    DiffDataInfo mDiffDataInfo GUARDED_BY(mLock);

    sp<KnockEpollThread> mKnockEpollThread;
    std::vector<SensorEntry*> mRecentQueue GUARDED_BY(mLock);

    int mKnockFeatureState = 0;

    int mRawDataNum;
    int mDeviceTx;
    int mDeviceRx;

    bool isFeatureReady();

    int identifyKnock();

    void enqueueInboundEventLocked(CommandEntry* entry) REQUIRES(mLock);

    bool isValidKnockArea(int surfaceOrientation, float x, float y);

    bool isNormalDoubleKnock(nsecs_t downTime) REQUIRES(mLock);

    bool isVeryQuickTap(nsecs_t downTime) REQUIRES(mLock);

    bool isNearLastDownArea(float x, float y) REQUIRES(mLock);

    void setKnockFeatureState(int32_t state);

    void setKnockDeviceProperty(int32_t property);

    void setKnockValidRect(int32_t left, int32_t top, int32_t right, int32_t bottom);

    void setKnockInValidRect(int32_t left, int32_t top, int32_t right, int32_t bottom);

    void setKnockScoreThreshold(float score);

    int updateAlgorithmPath(const char* dlcPath);

    void notifyMiuiInputConfigChanged(int32_t configType, int32_t changes);

    bool isSensorMatch() REQUIRES(mLock);

    int setDlcPath(const char* dlcPath);

    int initSnpe();

    void onConfigurationChanged(InputConfigTypeEnum type, int32_t changes);

    std::function<void(InputConfigTypeEnum, int32_t)> mOnConfigurationChangedListener =
                [this](InputConfigTypeEnum typeEnum, int32_t changes) { onConfigurationChanged(typeEnum, changes); };
};

class KnockEpollThread : public Thread {
public:
    explicit KnockEpollThread(KnockService* knockService);

    ~KnockEpollThread();

    void enableSensor();

    void disableSensor();

    virtual void dump(std::string& dump);

    int mSensorFd;
    int mTouchFd;

private:
    struct SensorEngine {
        SensorManager* sensorManager;
        const Sensor* knockSensor;
        sp<SensorEventQueue> sensorQueue;
    };

    const int32_t SENSOR_TYPE_KNOCK = 33171038;

    virtual bool doThreadLoop();

    virtual bool threadLoop();

    KnockService* mKnockListener;
    SensorEngine mSensorEngine;
};

} // namespace android
#endif // _UI_KNOCK_SERVICE_H
