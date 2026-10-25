#include <utils/String8.h>
#include <utils/threads.h>
#include <utils/Errors.h>

#include "octvm.h"
#include "utils/cJSON.h"
#include "PowerModeHandler.h"
#include "drv/platform_power.h"

using namespace android;

class AutoSavingThread : public Thread {

public:
    AutoSavingThread(struct autosave_params *params);
    virtual ~AutoSavingThread() {}

    virtual bool threadLoop();

    void dumpParams(int fd);
    bool setWindowLength(unsigned long windowLength);
    bool setForceIdleOffPct(int forceIdleOffPct);
    bool setForceBusyOffPct(int forceBusyOffPct);
    bool updateThreadParam(struct autosave_params *newParams);
    bool updateThreadParam(cJSON *config_data);
    void onPowerModeChanged(PowerModeHandler* pModeHandler);

private:
    bool mEnabled;
    unsigned int mCpuNum;
    unsigned int *mCpuStateArray;
    unsigned long *mLastAllCpuTimeArray;
    unsigned long mCycleStartTime;// in ms

    Mutex mLock;
    unsigned long mWindowLength; // in ms
    int mForceIdleOffPct;        // 1~100
    int mForceBusyOffPct;        // 1~100
};