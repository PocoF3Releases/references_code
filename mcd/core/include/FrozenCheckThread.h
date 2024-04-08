#include <utils/String8.h>
#include <utils/threads.h>
#include <utils/Errors.h>
#include <utils/Vector.h>
#include "octvm.h"
#include "drv/platform_power.h"

using namespace android;

class FrozenCheckThread : public Thread {

public:
    FrozenCheckThread();
    virtual ~FrozenCheckThread() {}

    virtual bool threadLoop();

    void dumpParams(int fd);
    bool addUidToCheckList(int uid);
    bool delUidFromCheckList(int uid);

private:
    bool mEnabled;
    unsigned long sleepInterval;
    Mutex mLock;
    Vector<int> mCheckUidVector;
};