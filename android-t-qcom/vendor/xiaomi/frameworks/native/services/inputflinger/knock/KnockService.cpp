#define LOG_TAG "KnockService"

#include "KnockService.h"
#include <android-base/stringprintf.h>
#include <android/dlext.h>
#include <dlfcn.h>

#include "MiInputManager.h"

#define EPOLL_EVENT_SIZE 10

// algorithm result knock, repeate check quick slide time
constexpr nsecs_t MOTION_KNOCK_REPEAT_CHECK_TIMEOUT = 30 * 1000000LL;
//快速点击140～200ms的，上次touch直接算touch，上次叩击的，本次仍要走算法流程
constexpr nsecs_t MAX_KNOCK_INTERVAL_CHECK_TIME = 200 * 1000000LL;
//快速点击140ms内的，第二次直接算触摸
constexpr nsecs_t MIN_KNOCK_INTERVAL_CHECK_TIME = 140 * 1000000LL;
// 50ms内有sensor数据，就认为sensor 震动符合阈值，底层30ms报一次
constexpr nsecs_t SENSOR_VALID_TIME = 50000000LL;

const char* TOUCH_RX_PATH = "/sys/class/touch/touch_dev/touch_thp_rx_num";
const char* TOUCH_TX_PATH = "/sys/class/touch/touch_dev/touch_thp_tx_num";

static const bool kKnockUpdateNode = property_get_bool("persist.knock.update_node", true);
const char* MMAP_PATH = kKnockUpdateNode ? "/dev/xiaomi-touch-knock" : "/dev/xiaomi-touch";
const char* DIFFDATA_NOTIFY =
        kKnockUpdateNode ? "/dev/xiaomi-touch-knock" : "/sys/class/touch/touch_dev/clicktouch_raw";

//是否收集数据，收集数据的模式下，叩击是无效的，不走算法
const char* PROPERTY_KNOCK_OPEN_DATA_COLLECTION = "persist.knock.open_data_collection";
//数据收集需要保存的帧数
const char* PROPERTY_KNOCK_SAVE_FRAME_COUNT = "persist.knock.save_frame_count";
//非收集模式下，等待有效帧到来的最大时间
const char* PROPERTY_KNOCK_WAIT_USE_FRAME_TIME = "persist.knock.wait_use_frame_time";

// USE_FRAME用于算法d计算，从0开始算
// SAVE_FRAME 需要驱动上报的数据总数，SAVE_FRAME>=USE_FRAME+1
static const bool kKnockFourFrame = property_get_bool("persist.knock.four_frame", false);
int32_t USE_FRAME = kKnockFourFrame ? 3 : 2;
int32_t SAVE_FRAME = kKnockFourFrame ? 4 : 3;

//默认有效帧最大等待时间
#define DEFAULT_WAIT_USE_FRAME_TIME 15
//算法所需数据的行
constexpr int32_t VALID_DATA_LINE = 7;
//算法所需数据的列
constexpr int32_t VALID_DATA_ROW = 7;
static const bool kKnockSupport = property_get_bool("persist.knock.support", false);
//算法需要的数据长度，行*列*帧数
int32_t VALID_RAW_DATA_LENTH = VALID_DATA_LINE * VALID_DATA_ROW * (USE_FRAME + 1);

#define INDENT2 "    "

using android::base::StringPrintf;

#undef ALOGD
#define ALOGD(...)                                          \
    if (MiInputManager::getInstance()->getInputReaderAll()) \
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

namespace android {

// --- EventEntry ---
KnockService::SensorEntry::SensorEntry(nsecs_t time, int data) : time(time), data(data) {}

KnockService::SensorEntry::~SensorEntry() {}

static inline const char* toString(bool value) {
    return value ? "true" : "false";
}

extern "C" KnockServiceInterface* createKnockService(InputListenerInterface* listener) {
    return new KnockService(listener);
}

KnockService::KnockService(InputListenerInterface* listener)
            : mMiuiInputMapper(listener) {
    ALOGI("KnockService onCreate");
    mLooper = new Looper(false);
    if (kKnockSupport) {
        ALOGI("KnockService Support");
        const std::string instance = std::string() + IKnockAlgorithm::descriptor + "/default";
        mKnockAlgorithm = IKnockAlgorithm::fromBinder(
                SpAIBinder(AServiceManager_getService(instance.c_str())));
    }
    property_get(PROPERTY_DISPLAY_VERSION, mDisplayVersion, "0");
    mKnockServiceState = KNOCK_SERVICE_STATE_FINISH;
    mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_TOUCH;
    mKnockRegion.reset();
    mKnockSnpeDlcInitResult = false;
    mKnockState.collectData = property_get_int32(PROPERTY_KNOCK_OPEN_DATA_COLLECTION, 0);
    mKnockState.saveFrameCount = property_get_int32(PROPERTY_KNOCK_SAVE_FRAME_COUNT, SAVE_FRAME);
    mKnockState.waitUseFrameTime =
            (property_get_int32(PROPERTY_KNOCK_WAIT_USE_FRAME_TIME, DEFAULT_WAIT_USE_FRAME_TIME)) *
            1000000LL;
    MiInputManager::getInstance()->addOnConfigurationChangedListener(
                mOnConfigurationChangedListener);
}

KnockService::~KnockService() {
    ALOGI("KnockService onDestory");
}

status_t KnockService::start() {
    if (mThread) {
        return ALREADY_EXISTS;
    }
    mThread = std::make_unique<InputThread>(
            "KnockService", [this]() { loopOnce(); }, [this]() { mLooper->wake(); });
    return OK;
}

status_t KnockService::stop() {
    if (mThread && mThread->isCallingThread()) {
        ALOGE("KnockService cannot be stopped from its own thread!");
        return INVALID_OPERATION;
    }
    mThread.reset();
    return OK;
}

void KnockService::setKnockValidRect(int32_t left, int32_t top, int32_t right, int32_t bottom) {
    mKnockRegion.knockMaxValidLeft = left;
    mKnockRegion.knockMaxValidTop = top;
    mKnockRegion.knockMaxValidRight = right;
    mKnockRegion.knockMaxValidBottom = bottom;
}

void KnockService::setKnockInValidRect(int32_t left, int32_t top, int32_t right, int32_t bottom) {
    mKnockRegion.knockInValidLeft = left;
    mKnockRegion.knockInValidTop = top;
    mKnockRegion.knockInValidRight = right;
    mKnockRegion.knockInValidBottom = bottom;
}

void KnockService::setKnockFeatureState(int32_t state) {
    mKnockFeatureState = state;
    if (mKnockEpollThread != nullptr) {
        if (mKnockFeatureState) {
            mKnockEpollThread->enableSensor();
        } else {
            mKnockEpollThread->disableSensor();
        }
    }
    if (mKnockFeatureState && mSnpeNotInit) {
        CommandEntry* newEntry = new CommandEntry(nullptr, TYPE_INIT_SNPE);
        enqueueInboundEventLocked(newEntry);
        mSnpeNotInit = false;
    }
}

void KnockService::setKnockDeviceProperty(int32_t property) {
    mKnockFeatureOpen = ((property & KNOCK_FEATRUE_OPEN) != 0);
    if (mKnockEpollThread == nullptr) {
        mKnockEpollThread = new KnockEpollThread(this);
        int result = mKnockEpollThread->run("KnockEpollThread", PRIORITY_URGENT_DISPLAY);
        if (result) {
            ALOGE("Could not start KnockEpollThread thread due to error %d.", result);
            mKnockEpollThread->requestExit();
        }
    }
}

void KnockService::setKnockScoreThreshold(float score) {
    mKnockScoreThreshold = score;
}

bool KnockService::isOpenNativeKnock() {
    if (!kKnockSupport) {
        return false;
    }
    if (mKnockState.collectData) {
        return true;
    }
    if (mKnockSnpeDlcInitResult) {
        if (mKnockFeatureOpen && mKnockFeatureState) {
            return true;
        }
    }
    return false;
}

KnockService::CommandEntry::CommandEntry(NotifyMotionArgs* args, int32_t type)
      : mMotionArgs(args), type(type) {}

KnockService::CommandEntry::~CommandEntry() {
    if (mMotionArgs != nullptr) {
        delete mMotionArgs;
    }
}

int KnockService::updateAlgorithmPath(const char* dlcPath) {
    int result = setDlcPath(dlcPath);
    if (result != -1) {
        ALOGE("updateDlcPath dlc success");
        return 1;
    } else {
        result = setDlcPath(DEFAULT_DLC_PATH);
        if (result != -1) {
            ALOGE("updateDlcPath dlc fail path:%s, use default dlc success", dlcPath);
        } else {
            ALOGE("updateDlcPath dlc fail path:%s, use default dlc fail", dlcPath);
        }
    }
    return -1;
}

int KnockService::setDlcPath(const char* dlcPath) {
    ALOGI("dlcPath : %s", dlcPath);
    int dlcFd = open(dlcPath, O_RDONLY);
    if (dlcFd < 0) {
        ALOGE("setDlcPath dlcpath:%s open fail", dlcPath);
        close(dlcFd);
        return -1;
    }
    close(dlcFd);
    std::strcpy(mDlcPath, dlcPath);
    return 0;
}

int KnockService::initSnpe() {
    if (mKnockAlgorithm) {
        int result = 0;
        mKnockAlgorithm->initSnpe(mDlcPath, &result);
        if (result == -1) {
            ALOGE("setDlcPath init snpe fail");
            return -1;
        }
        mKnockSnpeDlcInitResult = true;
    } else {
        mKnockSnpeDlcInitResult = false;
    }
    return 0;
}

void KnockService::notifyKeyDataReady(bool keyDown) {
    if (!mKnockSnpeDlcInitResult) {
        ALOGD("notifyKeyDataReady knock service init fail");
        return;
    }

    mLock.lock();
    if (keyDown) {
        if (mKnockServiceState == KNOCK_SERVICE_STATE_RECEIVING) {
            //处于接收状态，但没有timeout到processing的状态，注入read_diff事件，开始读取数据
            CommandEntry* newEntry = new CommandEntry(nullptr, TYPE_START_READ_DIFF);
            enqueueInboundEventLocked(newEntry);
        }
    } else {
        if (mKnockServiceState == KNOCK_SERVICE_STATE_RECEIVING) {
            //针对没down的情况或有down但状态不对，直接下放事件
            mLooper->wake();
        }
    }
    mLock.unlock();
}

void KnockService::notifyDeviceTxRx(int tx, int rx) {
    mDeviceRx = rx;
    mDeviceTx = tx;
    char dataOut[100];
    //存储到指定位置
    sprintf(dataOut, "tx:%-2d,rx:%-2d", tx, rx);
    int fd = open("data/knock_tx_rx", O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if (fd < 0) {
        ALOGE("open knock_tx_rx file failed");
    } else {
        lseek(fd, 0, SEEK_END);
        int ret = write(fd, dataOut, strlen(dataOut));
        if (ret < 0) {
            ALOGE("write once knock_tx_rx failed");
        } else {
            ALOGD("write once  %d to knock_tx_rx\n", ret);
        }
        close(fd);
    }
}

void KnockService::notifyRawTouch(int* data) {
    if(!mKnockSnpeDlcInitResult) {
        return;
    }
    mLock.lock();
    if (mLastDownInfo.upTime == -1) {
        //本次up事件已经被消耗掉，非第一帧
        mRawDataNum++;
    } else if (systemTime(CLOCK_MONOTONIC) - mLastDownInfo.upTime > 30000000) {
        //大于100ms，本次是第一帧
        mRawDataNum = 0;
        mLastDownInfo.upTime = -1;
    } else {
        mLock.unlock();
        //小于100ms，有可能是up报的那帧，直接丢弃
        return;
    }

    ALOGD("nofityRawTouch rawDataNum:%d,USE_FRAME:%d", mRawDataNum, USE_FRAME);
    if (mRawDataNum == 0) {
        int maxValue = 0;
        for (int i = 0; i < mDeviceRx; i++) {
            for (int j = 0; j < mDeviceTx; j++) {
                if (maxValue < data[i * mDeviceTx + j]) {
                    maxValue = data[i * mDeviceTx + j];
                    mDiffDataInfo.centerX = j;
                    mDiffDataInfo.centerY = i;
                }
            }
        }
        //边界补0的行数
        int appendCount = 1;
        bool valid = mDiffDataInfo.centerX - 3 + appendCount >= 0 &&
                mDiffDataInfo.centerY - 3 + appendCount >= 0 &&
                mDiffDataInfo.centerX + 3 - appendCount < mDeviceTx &&
                mDiffDataInfo.centerY + 3 - appendCount < mDeviceRx;
        if (valid) {
            mDiffDataInfo.collectState = RAW_DATA_COLLECT_RECEIVING;
        } else {
            mDiffDataInfo.collectState = RAW_DATA_COLLECT_PENDING;
        }
    }

    if (mDiffDataInfo.collectState == RAW_DATA_COLLECT_RECEIVING && mRawDataNum <= USE_FRAME) {
        int left = mDiffDataInfo.centerX - 3;
        int top = mDiffDataInfo.centerY - 3;

        float currentFrameSum = 0;
        for (int i = 0; i < VALID_DATA_LINE; i++) {
            for (int j = 0; j < VALID_DATA_ROW; j++) {
                int value = 0;
                if (i + top >= 0 && top + i < mDeviceRx && left + j >= 0 && left + j < mDeviceTx) {
                    value = data[(i + top) * mDeviceTx + j + left];
                }
                int diffDataIndex = (i * 7 + j) * (USE_FRAME + 1) + mRawDataNum;
                if (value >= 1200) {
                    //数据存储方式[0,1,2,0,1,2.......]方式存储
                    mDiffDataInfo.diffdataArray[diffDataIndex] = 1200;
                } else if (value <= -1200) {
                    mDiffDataInfo.diffdataArray[diffDataIndex] = -1200;
                } else {
                    mDiffDataInfo.diffdataArray[diffDataIndex] = value;
                }
                //对每一帧数据进行求和
                if (mDiffDataInfo.diffdataArray[diffDataIndex] > 0) {
                    currentFrameSum += mDiffDataInfo.diffdataArray[diffDataIndex];
                } else {
                    currentFrameSum += -mDiffDataInfo.diffdataArray[diffDataIndex];
                }
            }
        }
        //若一帧的数据总和小于500，则将状态赋值为-1,不再处理剩下帧数的数据
        if (currentFrameSum <= 500) {
            mDiffDataInfo.collectState = RAW_DATA_COLLECT_FAIL;
        } else if (mRawDataNum == USE_FRAME) {
            mDiffDataInfo.collectState = RAW_DATA_COLLECT_SUCCESS;
        }
    } else {
        ALOGI("knock edge invalid");
    }

    if (mKnockServiceState == KNOCK_SERVICE_STATE_RECEIVING && mRawDataNum == USE_FRAME) {
        //处于接收状态，但没有timeout到processing的状态，注入read_diff事件，开始读取数据
        CommandEntry* newEntry = new CommandEntry(nullptr, TYPE_START_READ_DIFF);
        enqueueInboundEventLocked(newEntry);
    }
    if (!mKnockState.collectData || mRawDataNum >= mKnockState.saveFrameCount) {
        mLock.unlock();
        return;
    }
    // save diffdata for knock app start
    int ret;
    int fd_out;
    char dataOut[10240];
    char data_tmp[1280];
    char data_index[10];
    sprintf(dataOut, "frame:%-2d\n", mRawDataNum);
    memset(data_tmp, 0, sizeof(data_tmp));
    for (int i = 0; i < mDeviceRx; i++) {
        for (int j = 0; j < mDeviceTx; j++) {
            if (j == mDeviceTx - 1) {
                sprintf(data_index, "%-4d\n", data[i * mDeviceTx + j]);
            } else {
                sprintf(data_index, "%-4d,", data[i * mDeviceTx + j]);
            }
            strcat(data_tmp, data_index);
            memset(data_index, 0, sizeof(data_index));
        }
        strcat(dataOut, data_tmp);
        memset(data_tmp, 0, sizeof(data_tmp));
    }

    if (mRawDataNum == 0) {
        fd_out = open("data/knock_raw_data", O_WRONLY | O_TRUNC | O_CREAT, 0666);
    } else {
        fd_out = open("data/knock_raw_data", O_WRONLY | O_CREAT, 0666);
    }

    if (fd_out < 0) {
        ALOGE("open once knock_raw_data failed");
    } else {
        lseek(fd_out, 0, SEEK_END);
        ret = write(fd_out, dataOut, strlen(dataOut));
        if (ret < 0) {
            ALOGE("write once knock_raw_data failed");
        } else {
            ALOGD("write once  %d to knock_raw_data\n", ret);
        }
        close(fd_out);
    }
    mLock.unlock();
}

bool KnockService::isIgnoreSensor() {
    return mKnockState.collectData;
}

int KnockService::getMaxInputFrame() {
    int maxInputFrame;
    if (mKnockState.saveFrameCount > USE_FRAME) {
        maxInputFrame = mKnockState.saveFrameCount;
    } else {
        maxInputFrame = USE_FRAME;
    }
    if (maxInputFrame < 0) {
        maxInputFrame = 0;
    }

    return maxInputFrame;
}

//记录下up的时间，超过100ms，则认为是一个新的down，不用在意是否有down。收到notifyRaw的时候，先将lastUp
//置成-1,等待下次up赋值 有可能一个中断上来down和up，即快速的按下抬起，也不需要判断
bool KnockService::isNeedSaveMotion(NotifyMotionArgs* args, int surfaceOrientation) {
    if (!mKnockSnpeDlcInitResult && !mKnockState.collectData) {
        return false;
    }
    mLock.lock();
    if (args->action == AMOTION_EVENT_ACTION_DOWN) {
        nsecs_t nowTime = systemTime(CLOCK_MONOTONIC);
        float downX = args->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_X);
        float downY = args->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_Y);
        if (!isValidKnockArea(surfaceOrientation, downX, downY)) {
            //有效区域(非边缘3x3):false 不做判断处理，默认触摸
            ALOGD("isNeedSaveMotion down not valid area");
            mKnockServiceState = KNOCK_SERVICE_STATE_FINISH;
            mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_TOUCH;
        } else if (isVeryQuickTap(nowTime)) {
            //非常快速点击，140ms之内的，本次事件不做处理，直接做touch
            ALOGD("isNeedSaveMotion near last down time less than 140ms , set finger touch");
            mKnockServiceState = KNOCK_SERVICE_STATE_FINISH;
            mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_TOUCH;
        } else if (!isNormalDoubleKnock(nowTime)) {
            //快速点击超过200ms，本次重新输入算法判断
            ALOGD("isNeedSaveMotion down interval more than 200ms , need algorithm judgment");
            // down事件，重置 将mKnockServiceState 事件重置 KNOCK_SERVICE_STATE_RECEIVING
            // 状态，等待超时或者153事件
            mKnockServiceState = KNOCK_SERVICE_STATE_RECEIVING;
            mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_TOUCH;
            CommandEntry* newEntry = new CommandEntry(nullptr, TYPE_ACTION_DOWN_DELAY);
            enqueueInboundEventLocked(newEntry);
        } else if (!isNearLastDownArea(downX, downY)) {
            //是否靠近上次点击(与上次down的距离)：false  快速点击,但与上次有效down间距过远，算touch
            ALOGD("isNeedSaveMotion near last down more than x distance, time in 200ms , set "
                  "finger touch");
            mKnockServiceState = KNOCK_SERVICE_STATE_FINISH;
            mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_TOUCH;
        } else {
            if (mCurrentToolTypeResult == KNOCK_ALGORITHM_RESULT_KNOCK) {
                //距离上次点击在140ms到200ms之间，上次是叩击，本次仍要算法运行一遍
                ALOGD("isNeedSaveMotion down interval time between 140ms and 200ms, last knock, "
                      "need algorithm judgment");
                mKnockServiceState = KNOCK_SERVICE_STATE_RECEIVING;
                mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_TOUCH;
                CommandEntry* newEntry = new CommandEntry(nullptr, TYPE_ACTION_DOWN_DELAY);
                enqueueInboundEventLocked(newEntry);
            } else {
                //距离上次点击在140ms到200ms之间，上次是touch，本次仍算touch
                ALOGD("isNeedSaveMotion down interval time between 140ms and 200ms, last touch set "
                      "touch");
                mKnockServiceState = KNOCK_SERVICE_STATE_FINISH;
                mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_TOUCH;
            }
        }
        //记录当前Down的事件
        mLastDownInfo.downTime = nowTime;
        mLastDownInfo.downX = downX;
        mLastDownInfo.downY = downY;
        mLastDownInfo.isMultipleTouch = false;
        mLastDownInfo.surfaceOrientation = surfaceOrientation;
        mLastDownInfo.isFastSlide = false;
        mLastDownInfo.isFastSlideChecking = true;
    } else if ((args->action & AMOTION_EVENT_ACTION_MASK) == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        //多指按下，如果此时处于receving状态，还没取diffdata数据，则直接取消
        if (mKnockServiceState == KNOCK_SERVICE_STATE_RECEIVING) {
            mLastDownInfo.isMultipleTouch = true;
            mLooper->wake();
        }
    } else if ((args->action & AMOTION_EVENT_ACTION_MASK) == AMOTION_EVENT_ACTION_MOVE) {
        //检测down下之后，30ms内的move事件速度超过2px/ms，touch快速滑动，如果识别成叩击，进行转换成touch
        if (mLastDownInfo.isFastSlideChecking &&
            (systemTime(CLOCK_MONOTONIC) - mLastDownInfo.downTime <
             MOTION_KNOCK_REPEAT_CHECK_TIMEOUT)) {
            float x = args->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_X);
            float y = args->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_Y);
            float dealtX = x - mLastDownInfo.downX;
            float dealtY = y - mLastDownInfo.downY;
            float speed = sqrt((dealtX * dealtX + dealtY * dealtY)) /
                    ((systemTime(CLOCK_MONOTONIC) - mLastDownInfo.downTime) / 1000000);
            if (speed > mKnockQuickMoveSpeed) {
                mLastDownInfo.isFastSlide = true;
                mLastDownInfo.isFastSlideChecking = false;
                if (mKnockServiceState == KNOCK_SERVICE_STATE_KNOCK_REPEAT_CHECKING) {
                    mLooper->wake();
                }
                ALOGI("Speed quick, fast slide = %f, mKnockQuickMoveSpeed = %f", speed,
                      mKnockQuickMoveSpeed);
            }
        } else {
            mLastDownInfo.isFastSlideChecking = false;
        }
    } else if (args->action == AMOTION_EVENT_ACTION_UP) {
        mLastDownInfo.upTime = systemTime(CLOCK_MONOTONIC);
        mDiffDataInfo.collectState = RAW_DATA_COLLECT_PENDING;
    }

    //只要状态不是finish，证明事件要么算法没结果，要么有事件没处理完毕，顺序依次执行，仍需加入队列
    if (mKnockServiceState != KNOCK_SERVICE_STATE_FINISH) {
        ALOGD("isNeedSaveMotion need save, mKnockServiceState : %d , action:%d", mKnockServiceState,
              args->action);
        NotifyMotionArgs* motionArgs = new NotifyMotionArgs(*args);
        CommandEntry* newEntry = new CommandEntry(motionArgs, TYPE_MODIFY_TOOL_TYPE);
        enqueueInboundEventLocked(newEntry);
        mLock.unlock();
        return true;
    } else {
        if (mCurrentToolTypeResult == KNOCK_ALGORITHM_RESULT_KNOCK) {
            ALOGD("isNeedSaveMotion not need save,change tool type : knock, action:%d",
                  args->action);
            for (uint32_t i = 0; i < args->pointerCount; i++) {
                args->pointerProperties[i].toolType = AMOTION_EVENT_TOOL_TYPE_KNOCK;
            }
        } else {
            ALOGD("isNeedSaveMotion not need save, not change tool type, action:%d", args->action);
        }
        mLock.unlock();
        return false;
    }
}

bool KnockService::isValidKnockArea(int surfaceOrientation, float x, float y) {
    if (!mKnockRegion.isInit()) {
        return false;
    }
    switch (surfaceOrientation) {
        case DISPLAY_ORIENTATION_90:
        case DISPLAY_ORIENTATION_270:
            return mKnockRegion.isInValidRegion(y, x);
        default:
            return mKnockRegion.isInValidRegion(x, y);
    }
    return false;
}

//是否小于200ms，大于140ms，正常的叩击区间
bool KnockService::isNormalDoubleKnock(nsecs_t downTime) {
    return downTime - mLastDownInfo.downTime < MAX_KNOCK_INTERVAL_CHECK_TIME;
}

//是否小于140ms，快速点击，本次直接算触摸
bool KnockService::isVeryQuickTap(nsecs_t downTime) {
    return downTime - mLastDownInfo.downTime < MIN_KNOCK_INTERVAL_CHECK_TIME;
}

bool KnockService::isNearLastDownArea(float x, float y) {
    return std::abs((int)(mLastDownInfo.downX - x)) < TWICE_DOWN_NEAR_DISTANCE &&
            std::abs((int)(mLastDownInfo.downY - y)) < TWICE_DOWN_NEAR_DISTANCE;
}

void KnockService::enqueueInboundEventLocked(CommandEntry* entry) {
    if (entry->type == TYPE_START_READ_DIFF || entry->type == TYPE_KNOCK_REPEAT_CHECK
            || entry->type == TYPE_INIT_SNPE) {
        //数据处理readdiff和read_check要立即处理
        mCommandQueue.enqueueAtHead(entry);
    } else {
        //其余数据加到队列尾部
        mCommandQueue.enqueueAtTail(entry);
    }
    if (entry->type == TYPE_START_READ_DIFF) {
        mLooper->wake();
    } else if (entry->type == TYPE_ACTION_DOWN_DELAY) {
        //第一个down事件会先导入一个timeout的计时，需要立即唤醒线程倒计时
        mLooper->wake();
    } else if (entry->type == TYPE_INIT_SNPE) {
        mLooper->wake();
    }
}

void KnockService::notifyAccelerometeSensorChange(float data) {
    nsecs_t nowTime = systemTime(CLOCK_MONOTONIC);
    ALOGD("notifyAccelerometeSensorChange  data:%f", data);
    mLock.lock();
    mRecentQueue.push_back(new SensorEntry(nowTime, data));
    if (mRecentQueue.size() > 3) {
        mRecentQueue.erase(mRecentQueue.begin());
    }
    mLock.unlock();
}

bool KnockService::isSensorMatch() {
    nsecs_t nowTime = systemTime(CLOCK_MONOTONIC);
    if (!mRecentQueue.empty()) {
        int verticalPoint;
        if (mLastDownInfo.surfaceOrientation == DISPLAY_ORIENTATION_0) {
            verticalPoint = mLastDownInfo.downY;
        } else {
            verticalPoint = mLastDownInfo.downX;
        }
        int minValue;
        if (verticalPoint < 550) {
            minValue = mSensorThresholdArray[0];
        } else if (verticalPoint < 1500) {
            minValue = mSensorThresholdArray[1];
        } else if (verticalPoint < 1900) {
            minValue = mSensorThresholdArray[2];
        } else {
            minValue = mSensorThresholdArray[3];
        }
        for (int i = mRecentQueue.size() - 1; i >= 0; --i) {
            SensorEntry* entry = mRecentQueue[i];
            if (nowTime - entry->time < SENSOR_VALID_TIME && entry->data >= minValue) {
                return true;
            }
        }
    }
    return false;
}

void KnockService::loopOnce() {
    nsecs_t nextWakeupTime = LONG_LONG_MAX;
    mLock.lock();
    if (!mCommandQueue.isEmpty()) {
        CommandEntry* mPendingCommand = mCommandQueue.dequeueAtHead();
        switch (mPendingCommand->type) {
            case TYPE_START_READ_DIFF: {
                ALOGD("loopOnce  TYPE_START_READ_DIFF");
                mKnockServiceState = KNOCK_SERVICE_STATE_PROCESSING;
                mLock.unlock();
                //此处耗时，从锁剥离
                int identifyResult = identifyKnock();
                mLock.lock();
                if (identifyResult == -1) {
                    //节点或算法异常，赋值为touch
                    mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_TOUCH;
                } else if (mLastDownInfo.isFastSlide) {
                    mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_TOUCH;
                } else if (mCurrentToolTypeResult == KNOCK_ALGORITHM_RESULT_KNOCK) {
                    CommandEntry* newEntry = new CommandEntry(nullptr, TYPE_KNOCK_REPEAT_CHECK);
                    enqueueInboundEventLocked(newEntry);
                    // down的时间+repeat_check时间，检查30ms内的move移动是否超阈值
                    nextWakeupTime = mLastDownInfo.downTime + MOTION_KNOCK_REPEAT_CHECK_TIMEOUT;
                    mKnockServiceState = KNOCK_SERVICE_STATE_KNOCK_REPEAT_CHECKING;
                }
                break;
            }
            case TYPE_KNOCK_REPEAT_CHECK: {
                ALOGD("loopOnce  TYPE_KNOCK_REPEAT_CHECK");
                if (mLastDownInfo.isFastSlide) {
                    ALOGI("loopOnce  TYPE_KNOCK_REPEAT_CHECK touch");
                    mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_TOUCH;
                }
                break;
            }
            case TYPE_MODIFY_TOOL_TYPE: {
                ALOGD("loopOnce  TYPE_MODIFY_TOOL_TYPE");
                mKnockServiceState = KNOCK_SERVICE_STATE_PROCESSING;
                NotifyMotionArgs* args = mPendingCommand->mMotionArgs;
                if (mCurrentToolTypeResult == KNOCK_ALGORITHM_RESULT_KNOCK) {
                    for (uint32_t i = 0; i < args->pointerCount; i++) {
                        args->pointerProperties[i].toolType = AMOTION_EVENT_TOOL_TYPE_KNOCK;
                    }
                }
                mMiuiInputMapper->notifyMotion(args);
                break;
            }
            case TYPE_ACTION_DOWN_DELAY: {
                ALOGD("loopOnce  TYPE_ACTION_DOWN_DELAY");
                nsecs_t currentTime = systemTime(CLOCK_MONOTONIC);
                if (!mLastDownInfo.isMultipleTouch) {
                    nextWakeupTime = currentTime + mKnockState.waitUseFrameTime;
                }
                break;
            }
            case TYPE_INIT_SNPE: {
                initSnpe();
                break;
            }
        }
        if (mCommandQueue.isEmpty() && (mPendingCommand->type == TYPE_MODIFY_TOOL_TYPE)) {
            if (mCommandQueue.isEmpty()) {
                mKnockServiceState = KNOCK_SERVICE_STATE_FINISH;
            } else {
                // flush之后，仍不为empty，证明flush期间插入了新事件，重新循环flush
                nextWakeupTime = LONG_LONG_MIN;
            }
        }
        delete mPendingCommand;
        if (nextWakeupTime == LONG_LONG_MAX) {
            nextWakeupTime = LONG_LONG_MIN;
        }
    }
    mLock.unlock();
    nsecs_t currentTime = systemTime(CLOCK_MONOTONIC);
    int timeoutMillis = toMillisecondTimeoutDelay(currentTime, nextWakeupTime);
    mLooper->pollOnce(timeoutMillis);
}

int KnockService::identifyKnock() {
    mLock.lock();
    if (mDiffDataInfo.collectState != RAW_DATA_COLLECT_SUCCESS) {
        mLock.unlock();
        ALOGI("identifyKnock mDiffDataInfo invalid");
        return 0;
    }
    nsecs_t currentTime1 = systemTime(CLOCK_MONOTONIC);
    double result = 0;
    if (mKnockAlgorithm) {
        std::vector<float> diffdataArray(mDiffDataInfo.diffdataArray,
                                         mDiffDataInfo.diffdataArray + 500);
        mLock.unlock();
        mKnockAlgorithm->predictCNN2(VALID_RAW_DATA_LENTH, diffdataArray, &result);
        mLock.lock();
    }
    nsecs_t currentTime2 = systemTime(CLOCK_MONOTONIC);
    long algorithmTime = currentTime2 - currentTime1;
    ALOGI("identifyKnock run algorithm score: %f , time:%ld", result, algorithmTime);
    // K2稳定版缺少mKnockScoreThreshold相关change
    if (result > mKnockScoreThreshold && !mKnockState.collectData) {
        if (isIgnoreSensor() || isSensorMatch()) {
            mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_KNOCK;
        } else {
            mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_TOUCH;
            ALOGI("algorithm result knock, but sensor touch");
        }
    } else {
        mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_TOUCH;
    }
    if (mLastDownInfo.isMultipleTouch) {
        mCurrentToolTypeResult = KNOCK_ALGORITHM_RESULT_TOUCH;
    }
    mLock.unlock();
    return 0;
}

void KnockService::dump(std::string& dump) {
    std::scoped_lock _l(mLock);
    dump += "\nKnock Service State:\n";
    dump += StringPrintf(INDENT2 "mKnockFeatureOpen: %s\n", toString(mKnockFeatureOpen));
    dump += StringPrintf(INDENT2 "mKnockSnpeDlcInitResult: %s\n",
                         toString(mKnockSnpeDlcInitResult));
    dump += StringPrintf(INDENT2 "ignoreSensor: %s\n", toString(isIgnoreSensor()));
    dump += StringPrintf(INDENT2 "Display Version: %s\n", mDisplayVersion);
    dump += StringPrintf(INDENT2 "mDeviceRx: %d, mDeviceTx: %d\n", mDeviceRx, mDeviceTx);
    dump += StringPrintf(INDENT2 "Knock Valid Rect: [%d,%d],[%d,%d]\n",
                         mKnockRegion.knockMaxValidLeft, mKnockRegion.knockMaxValidTop,
                         mKnockRegion.knockMaxValidRight, mKnockRegion.knockMaxValidBottom);
    dump += StringPrintf(INDENT2 "Knock InValid Rect: [%d,%d],[%d,%d]\n",
                         mKnockRegion.knockInValidLeft, mKnockRegion.knockInValidTop,
                         mKnockRegion.knockInValidRight, mKnockRegion.knockInValidBottom);
    dump += StringPrintf(INDENT2 "Knock Score: %.3f\n", mKnockScoreThreshold);
    dump += StringPrintf(INDENT2 "isOpenNativeKnock: %s\n", toString(isOpenNativeKnock()));
    dump += StringPrintf(INDENT2 "mKnockFeatureState: %s\n", toString(mKnockFeatureState));
    dump += StringPrintf(INDENT2 "mSensorThresholdArray: [%d,%d,%d,%d] \n",
                         mSensorThresholdArray[0], mSensorThresholdArray[1],
                         mSensorThresholdArray[2], mSensorThresholdArray[3]);
    // for knock state
    dump += StringPrintf(INDENT2 "mKnockState.collectData: %s\n",
                         toString(mKnockState.collectData));
    dump += StringPrintf(INDENT2 "mKnockState.saveFrameCount: %d\n", mKnockState.saveFrameCount);
    dump += StringPrintf(INDENT2 "mKnockState.waitUseFrameTime: %ld\n",
                         (long)(mKnockState.waitUseFrameTime / 1000000));

    if (mKnockEpollThread != nullptr) {
        mKnockEpollThread->dump(dump);
    }
}

void KnockService::onConfigurationChanged(InputConfigTypeEnum config, int32_t changes) {
    if (config == INPUT_CONFIG_TYPE_KNOCK_CONFIG) {
        notifyMiuiInputConfigChanged(config, changes);
    }
}

void KnockService::notifyMiuiInputConfigChanged(int32_t configType, int32_t changes) {
    ALOGI("notifyMiuiInputConfigChanged configType:%d, changes:%d", configType, changes);
    if (configType == INPUT_CONFIG_TYPE_KNOCK_CONFIG) {
        if ((changes & CONFIG_KNOCK_STATE) != 0) {
            setKnockFeatureState(MiInputManager::getInstance()->mInputKnockConfig->mKnockFeatureState);
        }
        if ((changes & CONFIG_KNOCK_DEVICE_PROPERTY) != 0) {
            setKnockDeviceProperty(MiInputManager::getInstance()->mInputKnockConfig->mKnockDeviceProperty);
        }
        if ((changes & CONFIG_KNOCK_VALID_RECT) != 0) {
            setKnockValidRect(MiInputManager::getInstance()->mInputKnockConfig->mKnockValidRectLeft,
                              MiInputManager::getInstance()->mInputKnockConfig->mKnockValidRectTop,
                              MiInputManager::getInstance()->mInputKnockConfig->mKnockValidRectRight,
                              MiInputManager::getInstance()->mInputKnockConfig->mKnockValidRectBottom);
        }
        if ((changes & CONFIG_KNOCK_INVALID_RECT) != 0) {
            setKnockInValidRect(MiInputManager::getInstance()->mInputKnockConfig->mKnockInValidRectLeft,
                                MiInputManager::getInstance()->mInputKnockConfig->mKnockInValidRectTop,
                                MiInputManager::getInstance()->mInputKnockConfig->mKnockInValidRectRight,
                                MiInputManager::getInstance()->mInputKnockConfig->mKnockInValidRectBottom);
        }
        if ((changes & CONFIG_KNOCK_ALGORITHM_PATH) != 0) {
            updateAlgorithmPath(MiInputManager::getInstance()->mInputKnockConfig->mKnockAlgorithmPath);
        }
        if ((changes & CONFIG_KNOCK_SCORE_THRESHOLD) != 0) {
            setKnockScoreThreshold(MiInputManager::getInstance()->mInputKnockConfig->mKnockScoreThreshold);
        }
        if ((changes & CONFIG_KNOCK_USE_FRAME) != 0) {
            mKnockUseFrame = MiInputManager::getInstance()->mInputKnockConfig->mKnockUseFrame;
        }
        if ((changes & CONFIG_KNOCK_QUICK_MOVE_SPEED) != 0) {
            mKnockQuickMoveSpeed = MiInputManager::getInstance()->mInputKnockConfig->mKnockQuickMoveSpeed;
        }
        if ((changes & CONFIG_KNOCK_SENSOR_THRESHOLD) != 0) {
            mSensorThresholdArray = MiInputManager::getInstance()->mInputKnockConfig->mKnockSensorThreshold;
        }
    }
}

// --- KnockEpollThread ---
KnockEpollThread::KnockEpollThread(KnockService* knockService)
      : Thread(/*canCallJava*/ true), mKnockListener(knockService) {}

KnockEpollThread::~KnockEpollThread() {}

void KnockEpollThread::enableSensor() {
    if (mSensorEngine.knockSensor != nullptr) {
        auto status = mSensorEngine.sensorQueue->enableSensor(mSensorEngine.knockSensor);
        if (!status) {
            status = mSensorEngine.sensorQueue->setEventRate(mSensorEngine.knockSensor,
                                                             us2ns(1000000));
        }
        if (status) {
            ALOGE("enable knockSensor fail:%d", status);
        } else {
            ALOGD("enable knockSensor success");
        }
    } else {
        ALOGE("enable knockSensor fail, knockSensor null");
    }
    int fdRawTouchPoll = open(DIFFDATA_NOTIFY, O_RDWR);
    if (fdRawTouchPoll < 0) {
        ALOGE("can't open update_rawdata:%d\n", fdRawTouchPoll);
        close(fdRawTouchPoll);
    } else {
        char tempBuf[10];
        int maxInputFrame = mKnockListener->getMaxInputFrame();
        if (maxInputFrame >= 6) {
            ALOGE("enableSensor maxInputFrame more than 6 frame, not normal");
        }
        char dataOut[3];
        //存储到指定位置
        sprintf(dataOut, "%2d", maxInputFrame);
        ALOGI("enableSensor max input frame:%s", dataOut);
        write(fdRawTouchPoll, dataOut, 2);
        pread(fdRawTouchPoll, tempBuf, 2, 0);
        close(fdRawTouchPoll);
    }
}

void KnockEpollThread::disableSensor() {
    if (mSensorEngine.knockSensor != nullptr) {
        auto status = mSensorEngine.sensorQueue->disableSensor(mSensorEngine.knockSensor);
        if (status) {
            ALOGE("disable knockSensor fail:%d", status);
        } else {
            ALOGD("disable knockSensor success");
        }
    } else {
        ALOGE("disableSensor fail, knockSensor null");
    }
    int fdRawTouchPoll = open(DIFFDATA_NOTIFY, O_RDWR);
    if (fdRawTouchPoll < 0) {
        ALOGE("can't open update_rawdata:%d\n", fdRawTouchPoll);
        close(fdRawTouchPoll);
    } else {
        char tempBuf[10];
        write(fdRawTouchPoll, "0", 2);
        pread(fdRawTouchPoll, tempBuf, 2, 0);
        close(fdRawTouchPoll);
    }
}

bool KnockEpollThread::doThreadLoop() {
    int mEpollFd = epoll_create(EPOLL_EVENT_SIZE);
    if (!mKnockListener->isIgnoreSensor()) {
        mSensorEngine.sensorManager = &SensorManager::getInstanceForPackage(String16());
        mSensorEngine.knockSensor =
                mSensorEngine.sensorManager->getDefaultSensor(SENSOR_TYPE_KNOCK);
        if (mSensorEngine.knockSensor == nullptr) {
            ALOGE("knock sensor get fail");
            close(mEpollFd);
            return false;
        }

        mSensorEngine.sensorQueue = mSensorEngine.sensorManager->createEventQueue();
        if (mSensorEngine.sensorQueue != 0) {
            mSensorEngine.sensorQueue->incStrong(mSensorEngine.sensorManager);
        }

        mSensorFd = mSensorEngine.sensorQueue->getFd();
        struct epoll_event sensorEventItem = {};
        sensorEventItem.events = EPOLLWAKEUP | EPOLLPRI | EPOLLIN;
        sensorEventItem.data.fd = mSensorFd;
        int result = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mSensorFd, &sensorEventItem);
        if (result != 0) {
            ALOGE("knock epoll_ctl add sensor fd errno=%d", errno);
            close(mEpollFd);
            return false;
        }
    } else {
        ALOGE("knock threadLoop ignore sensor");
    }

    char tempBuf[10];
    short* buf;
    int tx = 0;
    int rx = 0;
    int* delta = NULL;
    void* memBase;

    int fdRawTouchPoll = open(DIFFDATA_NOTIFY, O_RDWR);
    if (fdRawTouchPoll < 0) {
        ALOGE("can't open rawdata notify, error=%d", fdRawTouchPoll);
        close(fdRawTouchPoll);
        return 0;
    } else {
        write(fdRawTouchPoll, "0", 2);
        pread(fdRawTouchPoll, tempBuf, 2, 0);
    }

    int fdRxNum = open(TOUCH_RX_PATH, O_RDONLY);
    if (fdRxNum < 0) {
        ALOGE("can't get rx num fd\n");
        close(fdRawTouchPoll);
        close(fdRxNum);
        return false;
    }
    memset(tempBuf, 0x00, sizeof(tempBuf));
    pread(fdRxNum, tempBuf, 2, 0);
    rx = atoi(tempBuf);
    close(fdRxNum);

    int fdTxNum = open(TOUCH_TX_PATH, O_RDONLY);
    if (fdTxNum < 0) {
        ALOGE("can't get tx num fd\n");
        close(fdRawTouchPoll);
        close(fdTxNum);
        return false;
    }

    memset(tempBuf, 0x00, sizeof(tempBuf));
    pread(fdTxNum, tempBuf, 2, 0);
    tx = atoi(tempBuf);
    close(fdTxNum);

    mKnockListener->notifyDeviceTxRx(tx, rx);

    delta = (int*)malloc(tx * rx * sizeof(int));

    int fdMap = open(MMAP_PATH, O_RDWR);
    if (fdMap < 0) {
        ALOGE("can't open mmap xiaomi-touch");
        close(fdRawTouchPoll);
        close(fdMap);
        return false;
    }

    memBase = mmap(0, tx * rx, PROT_READ | PROT_WRITE, MAP_SHARED, fdMap, 0);
    if (memBase == MAP_FAILED) {
        ALOGE("map touch rawdata failed\n");
        close(fdRawTouchPoll);
        close(fdMap);
        return false;
    }

    struct epoll_event touchEventItem = {};
    if (kKnockUpdateNode) {
        touchEventItem.events = EPOLLWAKEUP | EPOLLPRI | EPOLLIN;
    } else {
        touchEventItem.events = EPOLLWAKEUP | EPOLLPRI;
    }
    touchEventItem.data.fd = fdRawTouchPoll;
    int result = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fdRawTouchPoll, &touchEventItem);
    if (result != 0) {
        ALOGE("knock epoll_ctl rawdata notify errno=%d", errno);
        close(fdRawTouchPoll);
        close(fdMap);
        close(mEpollFd);
        return false;
    }
    mTouchFd = fdRawTouchPoll;
    struct epoll_event events[EPOLL_EVENT_SIZE];
    int eventCount = 0;
    for (;;) {
        eventCount = epoll_wait(mEpollFd, events, EPOLL_EVENT_SIZE, -1);
        if (eventCount < 0) {
            if (errno != EINTR) {
                ALOGW("poll failed (errno=%d)\n", errno);
                usleep(100000);
            }
        } else {
            for (int i = 0; i < eventCount; i++) {
                int fd = events[i].data.fd;
                if (!mKnockListener->isIgnoreSensor() && fd == mSensorFd) {
                    ASensorEvent event[1];
                    while (true) {
                        ssize_t actual = mSensorEngine.sensorQueue->read(event, 1);
                        if (actual > 0) {
                            mSensorEngine.sensorQueue->sendAck(event, actual);
                            mSensorEngine.sensorQueue->filterEvents(event, actual);
                            mKnockListener->notifyAccelerometeSensorChange(event[0].data[1]);
                        } else {
                            break;
                        }
                    }
                } else if (fd == mTouchFd) {
                    if (kKnockUpdateNode) {
                        buf = (short*)memBase;
                    } else {
                        pread(fdRawTouchPoll, tempBuf, 2, 0);
                        memset(tempBuf, 0x00, sizeof(tempBuf));
                        buf = (short*)memBase;
                    }
                    for (int j = 0; j < tx * rx; j++) {
                        delta[j] = (int)buf[j];
                    }
                    mKnockListener->notifyRawTouch(delta);
                } else {
                    ALOGE("epoll wait: noneneone---------mSensorFd:%d,mTouchFd:%d,fd:%d", mSensorFd,
                          mTouchFd, fd);
                }
            }
        }
    }

    close(fdRawTouchPoll);
    close(fdMap);
    close(mEpollFd);

    return true;
}


bool KnockEpollThread::threadLoop() {
    return doThreadLoop();
}

void KnockEpollThread::dump(std::string& dump) {
    dump += StringPrintf(INDENT2 "mSensorState: %s\n",
                         mSensorEngine.knockSensor != nullptr ? "normal" : "not found");
}

} // namespace android
