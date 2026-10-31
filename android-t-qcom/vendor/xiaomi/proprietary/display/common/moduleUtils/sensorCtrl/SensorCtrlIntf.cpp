#include "SensorCtrlIntf.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "SensorControl"
#endif

namespace android {
int DFLOG::loglevel = 1;
// ---------------------------------------------------------------------------
static void SensorRecordCallbackFunction(int event, void *user, const Event &info) {
    SensorCtrlIntf *source = (SensorCtrlIntf *) user;

    switch (event) {
        case SENSOR_TYPE_LIGHT_PRIMARY_SCREEN:
        case SENSOR_TYPE_FRONT_CCT:
        case SENSOR_TYPE_VIRTUAL_CCT:
        case SENSOR_TYPE_FOLD_STATUS:
        case SENSOR_TYPE_LIGHT_SECOND_SCREEN: {
            if (source) {
                source->sensorCallback(event, info);
            }
        } break;
        default:
            DF_LOGW("no match event=%d", event);
            break;
    }
}

ANDROID_SINGLETON_STATIC_INSTANCE(SensorCtrlIntf);

SensorCtrlIntf::SensorCtrlIntf()
{
    DF_LOGD("DisplaySensorCtrl ");
}

SensorCtrlIntf::~SensorCtrlIntf()
{
    DF_LOGD("~DisplaySensorCtrl ");
}

int SensorCtrlIntf::isSensorRunning(unsigned int type,int displayId)
{
    if (type >= SENSOR_MAX) {
        return 0;
    }

    return mSensorRunning[type][displayId];
}

int SensorCtrlIntf::checkActiveUserList(int type, SENSOR_USER user)
{
    return mSensorUsers[type][user];
}

void SensorCtrlIntf::addToActiveUser(int type, SENSOR_USER user)
{
    mSensorUsers[type][user] = 1;
}

void SensorCtrlIntf::removeFromActiveUser(int type, SENSOR_USER user)
{
    mSensorUsers[type][user] = 0;
}

int SensorCtrlIntf::sensorControlOP(int displayId, int op, int type, SENSOR_USER user)
{
    int ret = 0;
    int samplePeroidUs = ns2us(ms2ns(SENSOR_SAMPLE_RATE_MS));
    int maxBatchReportLatencyUs = 0;
    DF_LOGD("op=%d, type=%d, dispId %d, user %d", op, type, displayId, (int)user);
    switch (type) {
        case SENSOR_LIGHT: {
            if (0 == displayId && !mSensorInitialized[type][displayId]) {
                mSensorHandle[type][displayId] = new SensorControl(String16("DISPLAYFEATURE0"),
                    SENSOR_TYPE_LIGHT_PRIMARY_SCREEN, SensorRecordCallbackFunction, this);
                mSensorInitialized[type][displayId] = true;
            }
            if (1 == displayId && !mSensorInitialized[type][displayId]) {
                mSensorHandle[type][displayId] = new SensorControl(String16("DISPLAYFEATURE1"),
                    SENSOR_TYPE_LIGHT_SECOND_SCREEN, SensorRecordCallbackFunction, this);
                mSensorInitialized[type][displayId] = true;
            }

            if (op == SENSOR_START) {
                if (!checkActiveUserList(type, user)) {
                    mSensorUserCnt[type][displayId]++;
                    addToActiveUser(type, user);
                }
            } else if (op == SENSOR_STOP && mSensorUserCnt[type][displayId] > 0) {
                if (checkActiveUserList(type, user)) {
                    mSensorUserCnt[type][displayId]--;
                    removeFromActiveUser(type, user);
                }
            }

            if (op == SENSOR_START && !mSensorRunning[type][displayId]) {
                ret = mSensorHandle[type][displayId]->start(samplePeroidUs, maxBatchReportLatencyUs, false);
                if (ret) {
                    DF_LOGE("light sensor start output failed");
                } else {
                    mSensorRunning[type][displayId] = true;
                    DF_LOGD("light sensor start output");
                }
            } else if (op == SENSOR_STOP && mSensorRunning[type][displayId] && !mSensorUserCnt[type][displayId]) {
                ret = mSensorHandle[type][displayId]->stop();
                mSensorRunning[type][displayId] = false;
                DF_LOGD("light sensor stop output");
            }
        } break;
        case SENSOR_FRONT_CCT: {
            if (!mSensorInitialized[SENSOR_FRONT_CCT][displayId]) {
                mSensorHandle[SENSOR_FRONT_CCT][displayId] = new SensorControl(String16("DISPLAYFEATURE0"),
                    SENSOR_TYPE_VIRTUAL_CCT, SensorRecordCallbackFunction, this);
                mSensorInitialized[SENSOR_FRONT_CCT][displayId] = true;
            }

            if (op == SENSOR_START) {
                if (!checkActiveUserList(type, user)) {
                    mSensorUserCnt[type][displayId]++;
                    addToActiveUser(type, user);
                }
            } else if (op == SENSOR_STOP && mSensorUserCnt[type][displayId] > 0) {
                if (checkActiveUserList(type, user)) {
                    mSensorUserCnt[type][displayId]--;
                    removeFromActiveUser(type, user);
                }
            }
            if (op == SENSOR_START && !mSensorRunning[SENSOR_FRONT_CCT][displayId]) {
                ret = mSensorHandle[SENSOR_FRONT_CCT][displayId]->start(samplePeroidUs, maxBatchReportLatencyUs, false);
                mSensorRunning[SENSOR_FRONT_CCT][displayId] = true;
                DF_LOGD("cct sensor start output");
            } else if (op == SENSOR_STOP && mSensorRunning[SENSOR_FRONT_CCT][displayId]
                        && !mSensorUserCnt[SENSOR_FRONT_CCT][displayId]) {
                ret = mSensorHandle[SENSOR_FRONT_CCT][displayId]->stop();
                mSensorRunning[SENSOR_FRONT_CCT][displayId] = false;
                DF_LOGD("cct sensor stop output");
            }
        } break;
        case SENSOR_FOLD: {
            if (!mSensorInitialized[SENSOR_FOLD][displayId]) {
                mSensorHandle[SENSOR_FOLD][displayId] = new SensorControl(String16("DISPLAYFEATURE"));
                if (mSensorHandle[SENSOR_FOLD][displayId]) {
                    mSensorHandle[SENSOR_FOLD][displayId]->set(SENSOR_TYPE_FOLD_STATUS,
                            SensorRecordCallbackFunction, this, true);
                }
                mSensorInitialized[SENSOR_FOLD][displayId] = true;
            }
            if (op == SENSOR_START) {
                if (!checkActiveUserList(type, user)) {
                    mSensorUserCnt[type][displayId]++;
                    addToActiveUser(type, user);
                }
            } else if (op == SENSOR_STOP && mSensorUserCnt[type][displayId] > 0) {
                if (checkActiveUserList(type, user)) {
                    mSensorUserCnt[type][displayId]--;
                    removeFromActiveUser(type, user);
                }
            }
            if (op == SENSOR_START && !mSensorRunning[SENSOR_FOLD][displayId]) {
                ret = mSensorHandle[SENSOR_FOLD][displayId]->start(samplePeroidUs, maxBatchReportLatencyUs, false);
                mSensorRunning[SENSOR_FOLD][displayId] = true;
                DF_LOGD("fold sensor start output");
            } else if (op == SENSOR_STOP && mSensorRunning[SENSOR_FOLD][displayId] && !mSensorUserCnt[SENSOR_FOLD][displayId]) {
                ret = mSensorHandle[SENSOR_FOLD][displayId]->stop();
                mSensorRunning[SENSOR_FOLD][displayId] = false;
                DF_LOGD("fold sensor stop output");
            }
        } break;
        default:
            DF_LOGW("sensor type:%d is not supported!", type);
            break;
    }

    return ret;
}

void SensorCtrlIntf::setDisplayState(int display_state)
{
    mDisplayState = display_state;
}

void SensorCtrlIntf::registerCallBack(void *callback, void* user)
{
    if (mCallbackNum < sensor_user_max && callback) {
        mSensorCallBack[mCallbackNum] = (PFN_SENSORCTRL_CB)callback;
        mUser[mCallbackNum] = user;
        mCallbackNum++;
    }
}

void SensorCtrlIntf::sensorCallback(int event, const Event &info)
{
    int i = 0;

    for (i = 0; i <= mCallbackNum; i++) {
        if (i < sensor_user_max && mSensorCallBack[i]) {
            mSensorCallBack[i](event, info, mUser[i]);
        }
    }

    return;
}
} //namespace android
