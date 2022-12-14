#include "SceneRecognizer.h"

#define MAX_SEND_TIMES 2
#define RECOGNIZE_INTERVAL 2000000

namespace android {

int mRemainSendTimes = MAX_SEND_TIMES;

void SceneRecognizer::startThread() {
    ALOGD("RecognizeBossThread startThread");
    run("RecognizeBossThread", PRIORITY_URGENT_DISPLAY);
}

void SceneRecognizer::stopThread(){
    ALOGD("RecognizeBossThread stopThread");
    requestExitAndWait();
}

bool SceneRecognizer::threadLoop(){
    //ALOGD("RecognizeBossThread threadLoop");
    int sceneId = SCENEID_UNKNOWN;
    if (mRecognizeBossPatternCount > 0) {
        sceneId = SCENEID_YUANSHEN_BOSS;
    } else {
        sceneId = SCENEID_OTHERS;
    }
    // 检测到场景发生变化后，连续像joyose发送MAX_SEND_TIMES次消息
    if (sceneId != mCurrentScene) {
        mRemainSendTimes = MAX_SEND_TIMES;
        mCurrentScene = sceneId;
    }

    if (mRemainSendTimes > 0 && sceneId != SCENEID_UNKNOWN) {
        sendSceneIdToJoyose(sceneId);
        mRemainSendTimes--;
    }
    mRecognizeBossPatternCount = 0;
    usleep(RECOGNIZE_INTERVAL);
    return true;
}

void SceneRecognizer::updateTexSizeArr(const GLint& newWidth, const GLint& newHeight) {
    mTexSizeArr[0] = mTexSizeArr[1];
    mTexSizeArr[1] = std::make_pair(newWidth, newHeight);
}

void SceneRecognizer::updateEleCountArr(const GLsizei& newCount) {
    mEleCountArr[0] = mEleCountArr[1];
    mEleCountArr[1] = mEleCountArr[2];
    mEleCountArr[2] = mEleCountArr[3];
    mEleCountArr[3] = newCount;
}

void SceneRecognizer::checkBossPattern() {
    if((mTexSizeArr[0].first == 1024 && mTexSizeArr[0].second == 1024 && mTexSizeArr[1].first == 4 && mTexSizeArr[1].second == 4)
        && ((mEleCountArr[1] == 162 && mEleCountArr[2] == 54 && mEleCountArr[3] == 54)
            || (mEleCountArr[0] == 162 && mEleCountArr[1] == 54 && mEleCountArr[2] % 6 == 0 && mEleCountArr[3] == 54))) {
        mRecognizeBossPatternCount++;
    }
}

int SceneRecognizer::sendSceneIdToJoyose(int sceneId) {
    ALOGD("sendSceneIdToJoyose, sceneId: %d", sceneId);
    status_t ret;
    Parcel data, reply;
    sp<IBinder> remoteService = defaultServiceManager()->checkService(String16(REMOTE_SERVICE_NAME));
    if (remoteService == nullptr) {
        ALOGW("remote service not exist.");
        return SERVICE_NOT_EXIST;
    }

    data.writeInterfaceToken(String16(REMOTE_SERVICE_INTERFACE));
    data.writeInt32(1);
    std::string dataStr = "{\"sceneId\":" + std::to_string(sceneId) + "}";
    data.writeString16(String16(dataStr.c_str()));
    ret = remoteService->transact(HANDLE_GAME_BOOSTER_FOR_ONEWAY_CODE, data, &reply);
    if (ret != NO_ERROR) {
        ALOGE("migl:call remote service failed.");
        return CALL_REMOTE_FAILED;
    }
    return RECEIVE_EXCEPTION;
}

};