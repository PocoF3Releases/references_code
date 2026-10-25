#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <utils/Errors.h>
#include <utils/Thread.h>
#include <cutils/log.h>
#include "hooks.h"

#define SCENEID_UNKNOWN -1
#define SCENEID_OTHERS 1005
#define SCENEID_YUANSHEN_BOSS 1006

#define REMOTE_SERVICE_NAME         "xiaomi.joyose"
#define REMOTE_SERVICE_INTERFACE    "com.xiaomi.joyose.IJoyoseInterface"
#define HANDLE_GAME_BOOSTER_FOR_ONEWAY_CODE         IBinder::FIRST_CALL_TRANSACTION + 2

#define NON_ERROR                   0
#define CALL_FROM_SYSTEM            1
#define SERVICE_NOT_EXIST           2
#define CALL_REMOTE_FAILED          3
#define RECEIVE_EXCEPTION           4
#define RECEIVE_NULL_OBJECT         5
namespace android {

class SceneRecognizer:public Thread {
private:
    int mCurrentScene = SCENEID_UNKNOWN;
    int mRecognizeBossPatternCount = 0;
    std::pair<GLint,GLint> mTexSizeArr[2];
    GLsizei mEleCountArr[4];
public:
    SceneRecognizer():Thread(false){}
    virtual void updateTexSizeArr(const GLint& newWidth, const GLint& newHeight);
    virtual void updateEleCountArr(const GLsizei& newCount);
    virtual void checkBossPattern();
    virtual void startThread();
    virtual void stopThread();
    virtual bool threadLoop();
private:
    virtual int sendSceneIdToJoyose(int sceneId);
};
};