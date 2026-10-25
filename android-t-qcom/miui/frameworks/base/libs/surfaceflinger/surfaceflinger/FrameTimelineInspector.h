#ifndef SURFACE_FRAME_INSPECTOR_H_
#define SURFACE_FRAME_INSPECTOR_H_

#include <cutils/properties.h>
#include <utils/String8.h>
#include <utils/Timers.h>
#include <string>
#include <unordered_map>

#define NSEC_PER_DAY 86400000000000ULL

namespace android {

static const uint16_t gReportThreshold = 1000;
static const uint16_t gFirstFrameTokenThreshold = 50;
static const uint16_t gDisplayTokenDiffThreshold = 10;
static const uint16_t gSkipTokenAfterFirstFrame = 200;
static const uint16_t gReportScoreThreshold = 80;
static const uint8_t gMaxReportNumPerDay = 10;
static const char* gCloudConfigPath = "/data/mqsas/frametimeline/cloudcontroldata.txt";
static const String8 gOneTrackAppId("31000000889");
static const String8 gOneTrackClientName("FrameTimelineInspector");

class LayerInfo {
public:
    LayerInfo(std::string layerName):mName(layerName){};
    ~LayerInfo() = default;

    void updateDropedFrameData(uint64_t dropedNum, nsecs_t period);
    bool isNeedFrame(int64_t displayFrameToken);
    const String8 getOneTrackJsonData();
    void reset();

    inline void setCurrentToken(uint64_t token);
    inline void increaseDrewFrame() { mDrewFrameNum++; };
    inline std::string getName() const { return mName; };
    inline uint64_t getDrewFrameNum() const { return mDrewFrameNum; };

private:
    inline int64_t getTokenDiff();
    float getScore();

    std::string mName;
    uint64_t mDrewFrameNum = 0;
    uint64_t mDroppedFrameNum = 0;
    uint64_t mPerceptibleDroppedFrameNum = 0;
    uint64_t mSeriousDroppedFrameNum = 0;
    uint64_t mCurrentToken = 0;
    uint64_t mPreviousToken = 0;
};

class FrameTimelineInspector {
public:
    FrameTimelineInspector();
    ~FrameTimelineInspector() = default;

    static FrameTimelineInspector* getInstance();
    void monitorAppFrame(std::string layerName, int64_t token, int64_t displayFrameToken,
                        nsecs_t deadlineDelta, nsecs_t period, bool isAppDeadlineMissed);

private:
    int getDroppedNum(nsecs_t deadlineDelta, nsecs_t period);
    bool isReport();
    static void reportOneTrackEvent(std::unordered_map<std::string, LayerInfo>* layerMap);
    inline bool getPureLayerName(std::string& layerName);

    bool mEnable = false;
    static bool mIsReporting;
    std::unordered_map<std::string, LayerInfo> mLayerMap;
    nsecs_t mLastReportTime = 0;
};

} // namespace android

#endif /*SURFACE_FRAME_INSPECTOR_H_ */
