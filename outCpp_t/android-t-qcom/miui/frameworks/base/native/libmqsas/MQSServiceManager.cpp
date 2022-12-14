
#include <utils/String8.h>
#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include "MQSServiceManager.h"

namespace android {

ANDROID_SINGLETON_STATIC_INSTANCE(MQSServiceManager);

MQSServiceManager::MQSServiceManager() {
    mDeathObserver = new DeathObserver(*this);
    Mutex::Autolock _l(mLock);
    connectMQSServiceLocked();
    connectTrackBinderLocked();
}

void MQSServiceManager::reportSensorException(const String8& error) {
    Mutex::Autolock _l(mLock);
    connectMQSServiceLocked();
    if (mMQSService != NULL) {
        ALOGE("Sensor exception: %s\n", error.string());
        mMQSService->reportSimpleEvent(EVENT_SENSOR, String16(error));
    }
}

void MQSServiceManager::reportEventV2(const String8& module, const String8& info, const String8& appId, bool isGlobalNeed) {
    Mutex::Autolock _l(mLock);
    connectMQSServiceLocked();
    if (mMQSService != NULL) {
        ALOGE("report event v2: module is %s , appId is %s .\n  info is %s .\n",module.string(),appId.string(),info.string());
        if (isGlobalNeed){
            mMQSService->reportNativeEventV2(String16(module),String16(info),String16(appId),1);
        }else {
            mMQSService->reportNativeEventV2(String16(module),String16(info),String16(appId),0);
        }
    }
}

void MQSServiceManager::reportEventsV2(const String8& module, const std::vector<String16>& infolist, const String8& appId, bool isGlobalNeed) {
    Mutex::Autolock _l(mLock);
    connectMQSServiceLocked();
    if (mMQSService != NULL) {
        ALOGE("report events v2");
        if (isGlobalNeed){
            mMQSService->reportNativeEventsV2(String16(module),infolist ,String16(appId),1);
        }else {
            mMQSService->reportNativeEventsV2(String16(module),infolist ,String16(appId),0);
        }
    }
}

void MQSServiceManager::reportNativeFeatureEvent(const String8& message, const String8& details, const String8& type) {
    Mutex::Autolock _l(mLock);
    connectMQSServiceLocked();
    if (mMQSService != NULL) {
        ALOGE("Sensor exception: %s\n", details.string());
        mMQSService->reportNativeFeatureEvent(String16(message), String16(details), String16(type));
    }
}

void MQSServiceManager::appCaptureLog(const String8& module, const String8& actionList, const String8& logPath, const String8& servicesList) {
    Mutex::Autolock _l(mLock);
    connectMQSServiceLocked();
    if (mMQSService != NULL) {
        ALOGE("appCaptureLog: %s\n", module.string());
        mMQSService->appCaptureLog(String16(module), String16(actionList), String16(logPath), String16(servicesList));
    }
}

int MQSServiceManager::reportOneTrackEvent(const String8& appId, const String8& pkn, const String8& data, int flag) {
    Mutex::Autolock _l(mLock);
    connectTrackBinderLocked();
    if (mTrackBinder != NULL) {
        ALOGE("reportOneTrackEvent: %s\n", appId.string());
        mTrackBinder->trackEvent(String16(appId), String16(pkn), String16(data), flag);
        return 0;
    } else {
        ALOGE("reportOneTrackEvent: mTrackBinder == null");
        return 1;
    }
}

int MQSServiceManager::reportOneTrackEvents(const String8& appId, const String8& pkn, const std::vector<String16>& dataList, int flag) {
    Mutex::Autolock _l(mLock);
    connectTrackBinderLocked();
    if (mTrackBinder != NULL) {
        ALOGE("reportOneTrackEvents: %s\n", appId.string());
        mTrackBinder->trackEvents(String16(appId), String16(pkn), dataList, flag);
        return 0;
    } else {
        ALOGE("reportOneTrackEvents: mTrackBinder == null");
        return 1;
    }
}

void MQSServiceManager::connectMQSServiceLocked() {
    if (mMQSService == NULL) {
        const sp<IServiceManager> sm(defaultServiceManager());
        if (sm != NULL) {
            const String16 name("miui.mqsas.MQSService");
            mMQSService = interface_cast<IMQSService>(sm->checkService(name));
        }

        if (mMQSService != NULL) {
            #ifdef BINDER_STATIC_AS_BINDER
                IInterface::asBinder(mMQSService)->linkToDeath(mDeathObserver);
            #else
                mMQSService->asBinder()->linkToDeath(mDeathObserver);
            #endif
        }
    }
}

void MQSServiceManager::onMQSServiceDied() {
    Mutex::Autolock _l(mLock);
    mMQSService = NULL;
}

void MQSServiceManager::connectTrackBinderLocked() {
    ALOGE("Begin to connectTrackBinderLocked");
    if (mTrackBinder == NULL) {
        connectMQSServiceLocked();
        if (mMQSService != NULL) {
            ALOGE("Begin to invoke getOneTrackService()");
            mTrackBinder = interface_cast<ITrackBinder>(mMQSService->getOneTrackService());
        }

        if (mTrackBinder != NULL) {
            #ifdef BINDER_STATIC_AS_BINDER
                IInterface::asBinder(mTrackBinder)->linkToDeath(mDeathObserver);
            #else
                mTrackBinder->asBinder()->linkToDeath(mDeathObserver);
            #endif
        } else {
            ALOGE("getOneTrackService() failed");
        }
    }
}

void MQSServiceManager::onTrackBinderDied() {
    Mutex::Autolock _l(mLock);
    mTrackBinder = NULL;
}

}
