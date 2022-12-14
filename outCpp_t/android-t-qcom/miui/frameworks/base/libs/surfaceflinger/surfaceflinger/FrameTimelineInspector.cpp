#undef LOG_TAG
#define LOG_TAG "FrameTimelineInspector"

#include "FrameTimelineInspector.h"
#include "MQSServiceManager.h"
#include <android-base/stringprintf.h>
#include <log/log.h>
#include <fstream>
#include <thread>
#include <inttypes.h>

namespace android {

static FrameTimelineInspector gFrameTimelineInspector;
bool FrameTimelineInspector::mIsReporting = false;

void LayerInfo::reset() {
    mDrewFrameNum = 0;
    mDroppedFrameNum = 0;
    mSeriousDroppedFrameNum = 0;
    mPerceptibleDroppedFrameNum = 0;
    mCurrentToken = 0;
    mPreviousToken = 0;
}

void LayerInfo::updateDropedFrameData(uint64_t dropedNum, nsecs_t period) {
    mDroppedFrameNum += dropedNum;

    if (period < ms2ns(9)) {
        // 120HZ
        if (dropedNum >= 4) {
            mPerceptibleDroppedFrameNum += dropedNum;
            mSeriousDroppedFrameNum += dropedNum;
        } else if (dropedNum >= 3) {
            mPerceptibleDroppedFrameNum += dropedNum;
        }
    } else {
        // 60HZ and 90HZ
        if (dropedNum >= 3) {
            mPerceptibleDroppedFrameNum += dropedNum;
            mSeriousDroppedFrameNum += dropedNum;
        } else if (dropedNum >= 2) {
            mPerceptibleDroppedFrameNum += dropedNum;
        }
    }
}

float LayerInfo::getScore() {
    const float dropedFrameRateStandard = 2;
    const float perceptibleDropedFrameRateStandard = 0.4;
    const float seriousDroppedFrameRateStandard = 0.2;

    float allFrameNum  = mDrewFrameNum + mDroppedFrameNum;
    float dropedFrameRate = (float)(mDroppedFrameNum * 100) / allFrameNum;
    float perceptibleDropedFrameRate =(float)(mPerceptibleDroppedFrameNum * 100) / allFrameNum;
    float seriousDroppedFrameRate = (float)(mSeriousDroppedFrameNum * 100) / allFrameNum;

    float rateExceededNum = 0;

    if (dropedFrameRate >= dropedFrameRateStandard) {
        rateExceededNum++;
    }

    if (perceptibleDropedFrameRate >= perceptibleDropedFrameRateStandard) {
        rateExceededNum++;
    }

    if (seriousDroppedFrameRate >= seriousDroppedFrameRateStandard) {
        rateExceededNum++;
    }

    float score = ((float)(mDrewFrameNum) - (float)mPerceptibleDroppedFrameNum * 4
                    - (float)mSeriousDroppedFrameNum * 5) / (float)(allFrameNum) * 100
                    - rateExceededNum * 3;

    return score > 0 ? score : 0;
}

const String8 LayerInfo::getOneTrackJsonData() {
    float score = getScore();
    String8 jsonData;
    jsonData.appendFormat("{\"EVENT_NAME\":app_droped_frame,\"layer_name\":\"%s\","\
                          "\"score\":%.2f,\"drew_frame\":%" PRId64 ",\"df\":%" PRId64 ","\
                          "\"pdf\":%" PRId64 ",\"sdf\":%" PRId64 "}",
                          mName.c_str(), score, mDrewFrameNum, mDroppedFrameNum,
                          mPerceptibleDroppedFrameNum, mSeriousDroppedFrameNum);
    return jsonData;
}

void LayerInfo::setCurrentToken(uint64_t token) {
    mCurrentToken = token;
}

int64_t LayerInfo::getTokenDiff() {
    return int64_t(mCurrentToken - mPreviousToken);
}

bool LayerInfo::isNeedFrame(int64_t displayFrameToken) {
    if (mCurrentToken <= mPreviousToken) {
        return false;
    }

    if ((displayFrameToken - mCurrentToken >= gDisplayTokenDiffThreshold) &&
        (getTokenDiff() > gFirstFrameTokenThreshold)) {
        mPreviousToken = mCurrentToken + gSkipTokenAfterFirstFrame;
        return false;
    }

    if (mCurrentToken > mPreviousToken) {
        mPreviousToken = mCurrentToken;
        return true;
    }

    return false;
}

FrameTimelineInspector::FrameTimelineInspector() {
    bool enable = property_get_bool("persist.sys.frametimeline.inspector.enable", false);
    SLOGI("enable: %d", enable);
    if (enable) {
        std::ifstream inFile(gCloudConfigPath, std::ios_base::in);
        if (!inFile.is_open()) {
            SLOGE("open file failed: %s\n", strerror(errno));
        } else {
            std::string layerName;
            while (getline(inFile, layerName)) {
                if (getPureLayerName(layerName)) {
                    mLayerMap.insert(std::make_pair(layerName, LayerInfo(layerName)));
                    mEnable = true;
                    SLOGI("cloud setting layer Name %s", layerName.c_str());
                }
            }
        }
    }
}

FrameTimelineInspector* FrameTimelineInspector::getInstance() {
    return &gFrameTimelineInspector;
}

bool FrameTimelineInspector::getPureLayerName(std::string& layerName) {
    size_t end = layerName.find_last_of("#");
    if(end != std::string::npos) {
        layerName.erase(end, std::string::npos);
        return layerName.size() != 0;
    }
    return false;
}

void FrameTimelineInspector::monitorAppFrame(std::string layerName, int64_t token,
                                             int64_t displayFrameToken, nsecs_t deadlineDelta,
                                             nsecs_t period, bool isAppDeadlineMissed) {
    if (mEnable && !mIsReporting && getPureLayerName(layerName)) {
        auto iter = mLayerMap.find(layerName);
        if (iter != mLayerMap.end()) {

            auto layer = &iter->second;
            layer->increaseDrewFrame();
            layer->setCurrentToken(token);

            if (layer->isNeedFrame(displayFrameToken) && isAppDeadlineMissed) {
                int droped = getDroppedNum(deadlineDelta, period);
                if (droped > 0) {
                    layer->updateDropedFrameData(droped, period);
                }
            }

            if(isReport()) {
                mIsReporting = true;
                std::thread reportThread(reportOneTrackEvent, &mLayerMap);
                reportThread.detach();
            }
        }
    }
}

int FrameTimelineInspector::getDroppedNum(nsecs_t deadlineDelta, nsecs_t period) {
    if (deadlineDelta > 0) {
        int dropedFrames = int(deadlineDelta / period) + 1;
        if (dropedFrames > 0 && dropedFrames < 100) {
            return dropedFrames;
        }
    }
    return 0;
}

bool FrameTimelineInspector::isReport() {
    nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);

    if(mLastReportTime == 0) {
        mLastReportTime = now;
        return false;
    }

    if(now - mLastReportTime > NSEC_PER_DAY) {
        mLastReportTime = now;
        return true;
    }

    return false;
}

void FrameTimelineInspector::reportOneTrackEvent(
                            std::unordered_map<std::string, LayerInfo>* layerMap) {
    for (auto& iter : *layerMap) {
        auto& layerInfo = iter.second;
        if(layerInfo.getDrewFrameNum() != 0) {
            const String8& jsonData = layerInfo.getOneTrackJsonData();
            layerInfo.reset();

            int onetrackFlag = 0;
            try {
                onetrackFlag = MQSServiceManager::getInstance()
                            .reportOneTrackEvent(gOneTrackAppId, gOneTrackClientName, jsonData, 2);
            } catch (const char* msg) {
                ALOGE("reportOneTrackEvent failed: %s", msg);
            }

            if (0 != onetrackFlag) {
                ALOGE("reportOneTrackEvent failed");
            } else {
                ALOGI("reportOneTrackEvent sucessed, layer: %s", layerInfo.getName().c_str());
            }
        }
    }
    mIsReporting = false;
}

}