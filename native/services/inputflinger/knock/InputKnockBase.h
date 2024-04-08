
#ifndef _UI_INPUT_KNOCK_BASE_H
#define _UI_INPUT_KNOCK_BASE_H

#include <utils/RefBase.h>
#include "InputListener.h"
#include "MiInputManager.h"
#include "InputReaderBase.h"

namespace android {

struct NotifyMotionArgs;
class KnockServiceInterface : public virtual RefBase{
public:
    virtual status_t start() = 0;
    virtual status_t stop() = 0;
    virtual void dump(std::string& dump) = 0;
    virtual bool isOpenNativeKnock() = 0;
    virtual bool isNeedSaveMotion(NotifyMotionArgs* args, int surfaceOrientation) = 0;
    //key 153 事件通知数据准备可读
    virtual void notifyKeyDataReady(bool keyDown) = 0;
};

extern "C" KnockServiceInterface* createKnockService(InputListenerInterface* listener);
} // namespace android

#endif // _UI_INPUT_KNOCK_BASE_H