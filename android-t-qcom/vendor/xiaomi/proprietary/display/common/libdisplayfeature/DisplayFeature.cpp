#include <utils/Log.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include "DisplayFeature.h"

#define LOG_TAG "DisplayFeature"
#define LOG_NDEBUG 0


namespace android {

DisplayFeature::DisplayFeature()
    :mListener(0)
{
    ALOGD("%s", __func__);
}

DisplayFeature::~DisplayFeature()
{
    ALOGD("%s", __func__);
}

int DisplayFeature::setFunctionEnable(int displayId, int caseId,
    int modeId, int cookie)
{
    ALOGD("%s displayId=%d caseId=%d modeId=%d cookie=%d",
        __func__, displayId, caseId, modeId, cookie);

    int err = -1;
    const sp<IDisplayFeatureControl> service(getDisplayFeatureService());
    if (service != 0) {
        err = service->setFunctionEnable(displayId, caseId, modeId, cookie);

    }
    return err;
}

int DisplayFeature::setFeatureEnable(int caseId, int modeId, int cookie)
{
    ALOGD("%s modeId=%d cookie=%d", __func__, modeId, cookie);

    int err = -1;
    const sp<IDisplayFeatureControl> service(getDisplayFeatureService());
    if (service != 0) {
        err = service->setFeatureEnable(0, caseId, modeId, cookie);
    }
    return err;
}

void DisplayFeature::sendBrightness(int modeId, int cookie)
{
    const sp<IDisplayFeatureControl> service(getDisplayFeatureService());
    if (service != 0) {
        service->notify(0, service->DIMMING_BRIGHTNESS_STATE, modeId, cookie);
    }
}

void DisplayFeature::setListener(const sp<DisplayFeatureListener>& listener)
{
    ALOGD("setListener");
    Mutex::Autolock _l(mLock);
    mListener = listener;
}

void DisplayFeature::notify(int msg, int ext1, int ext2, const Parcel *obj)
{
    switch (msg) {
    case DISPLAY_NOP:
        break;
    case DISPLAY_ERROR:
        break;
    default:
        ALOGV("unrecognized message: (%d, %d, %d)", msg, ext1, ext2);
        break;
    }
    sp<DisplayFeatureListener> listener = mListener;
    if(listener != 0)
         listener->notify(msg, ext1, ext2, obj);
}

void DisplayFeature::died()
{
    ALOGV("died");
    notify(DISPLAY_ERROR, DISPLAY_ERROR_SERVER_DIED, 0);
}


// ---------------------------------------------------------------------------


// client singleton for binder interface to services
Mutex IDisplayDeathNotifier::sServiceLock;
sp<IDisplayFeatureControl> IDisplayDeathNotifier::sDisplayFeatureService;
sp<IDisplayDeathNotifier::DeathNotifier> IDisplayDeathNotifier::sDeathNotifier;
SortedVector< wp<IDisplayDeathNotifier> > IDisplayDeathNotifier::sObitRecipients;

// establish binder interface to dService
/*static*/const sp<IDisplayFeatureControl>
IDisplayDeathNotifier::getDisplayFeatureService()
{
    Mutex::Autolock _l(sServiceLock);
    if (sDisplayFeatureService == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("DisplayFeatureControl"));
            if (binder != 0) {
                break;
            }
            //ALOGW("Display Feature service not published, waiting... ");
            //usleep(500000); // 0.5 s
            return NULL;
        } while (true);

        if (sDeathNotifier == NULL) {
            sDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(sDeathNotifier);
        sDisplayFeatureService = interface_cast<IDisplayFeatureControl>(binder);
    }
    ALOGE_IF(sDisplayFeatureService == 0, "no display feature service!?");
    return sDisplayFeatureService;
}

/*static*/ void
IDisplayDeathNotifier::addObitRecipient(const wp<IDisplayDeathNotifier>& recipient)
{
    Mutex::Autolock _l(sServiceLock);
    ALOGD("addObitRecipient");
    sObitRecipients.add(recipient);
}

/*static*/ void
IDisplayDeathNotifier::removeObitRecipient(const wp<IDisplayDeathNotifier>& recipient)
{
    Mutex::Autolock _l(sServiceLock);
    ALOGD("removeObitRecipient");
    sObitRecipients.remove(recipient);
}

void
IDisplayDeathNotifier::DeathNotifier::binderDied(const wp<IBinder>& who __unused) {
    ALOGW("display feature died");

    // Need to do this with the lock held
    SortedVector< wp<IDisplayDeathNotifier> > list;
    {
        Mutex::Autolock _l(sServiceLock);
        sDisplayFeatureService.clear();
        list = sObitRecipients;
    }

    // Notify application when display feature dies.
    // Don't hold the static lock during callback in case app
    // makes a call that needs the lock.
    size_t count = list.size();
    for (size_t iter = 0; iter < count; ++iter) {
        sp<IDisplayDeathNotifier> notifier = list[iter].promote();
        if (notifier != 0) {
            notifier->died();
        }
    }
}

IDisplayDeathNotifier::DeathNotifier::~DeathNotifier()
{
    ALOGD("DeathNotifier::~DeathNotifier");
    Mutex::Autolock _l(sServiceLock);
    sObitRecipients.clear();
    if (sDisplayFeatureService != 0) {
        IInterface::asBinder(sDisplayFeatureService)->unlinkToDeath(this);
    }
}

}
