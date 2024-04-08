#define LOG_TAG "octvm_power"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "freezeProcessApi.h"
#include "FrozenCheckThread.h"

using namespace android;

FrozenCheckThread::FrozenCheckThread():
        mEnabled(true), sleepInterval(60*3){
        printf("FrozenCheckThread　create");
        mCheckUidVector.clear();
}
void FrozenCheckThread::dumpParams(int fd)
{
    String8 dBuffer;
    int uid;
    unsigned int i;
    Mutex::Autolock lock(mLock);
    dBuffer.appendFormat("\nCurrent FrozenCheckThread Parameters:\n");
    dBuffer.appendFormat("\t mEnabled =:%d\n", mEnabled);
    dBuffer.appendFormat("\t sleepInterval =:%lu\n", sleepInterval);
    dBuffer.appendFormat("\t mCheckUidVector.size =:%d\n", mCheckUidVector.size());
    for(i = 0; i < mCheckUidVector.size(); i++) {
        uid = mCheckUidVector.itemAt(i);
        dBuffer.appendFormat("\t mCheckUidVector.[%d] =: %d\n", i, uid);
    }
    dBuffer.appendFormat("\n");
    write(fd, dBuffer.string(), dBuffer.length());
}
bool FrozenCheckThread::addUidToCheckList(int uid) {
    Mutex::Autolock lock(mLock);
    if (!isRunning()) {
        run("frozenCheck", PRIORITY_NORMAL);
        printf("run frozenCheckThread");
    }
    Vector<int>::iterator it;
    for(it = mCheckUidVector.begin(); it != mCheckUidVector.end(); it++) {
        if (*it == uid) {
            return true;
        }
    }
    mCheckUidVector.push_back(uid);
    return true;
}
bool FrozenCheckThread::delUidFromCheckList(int uid) {
    Mutex::Autolock lock(mLock);
    for(unsigned int i = 0; i < mCheckUidVector.size(); i++) {
        int uidNew = mCheckUidVector.itemAt(i);
        if (uidNew == uid) {
            mCheckUidVector.erase(mCheckUidVector.begin() + i);
            return true;
        }
    }
    return false;
}
bool FrozenCheckThread::threadLoop() {
    while (!exitPending()) {
        sleep(sleepInterval);
        {
            Mutex::Autolock lock(mLock);
            if (mEnabled) {
                for (unsigned int i = 0; i < mCheckUidVector.size(); i++) {
                    int uid = mCheckUidVector.itemAt(i);
                    int status = fp_check_frozen_app_state(uid);
                    if (FROZEN_CHECK_THAWED_THEN_SHOULD_FROZEN == status) {
                        fp_check_frozen_app_thawed_then_frozen(uid);
                    } else if (FROZEN_CHECK_FROZEN == status) {
                        //ok ,do nothing
                    } else if (FROZEN_CHECK_NO_UID == status) {
                        //ok ,do nothing
                    } else if (FROZEN_CHECK_FROZEN_BUT_BEEN_KILLING == status) {
                        THAWED_APP(uid);
                        //thawed it ,then app will be killed, died events is catch by power keeper
                    } else if (FROZEN_CHECK_FROZEN_BUT_BEEN_KILLED == status) {
                        //ok ,do nothing
                    }
                }
            }
        }
    }
    return false;
}