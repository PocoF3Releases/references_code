#ifndef _DISPLAY_FEATURE_H_
#define _DISPLAY_FEATURE_H_

#include <utils/threads.h>
#include <utils/SortedVector.h>
#include "IDisplayFeatureControl.h"
#include "display_effects.h"

namespace android {

enum display_error_type {
    // 0xx
    DISPLAY_ERROR_UNKNOWN = 1,
    // 1xx
    DISPLAY_ERROR_SERVER_DIED = 100,
    // 2xx

};

enum display_event_type {
    DISPLAY_NOP               = 0,
    DISPLAY_ERROR             = 100,
    DISPLAY_INFO              = 200,
};

class DisplayFeatureListener: virtual public RefBase
{
public:
    virtual void notify(int msg, int ext1, int ext2, const Parcel *obj) = 0;
};

class IDisplayDeathNotifier: virtual public RefBase
{
public:
    IDisplayDeathNotifier() { addObitRecipient(this); }
    virtual ~IDisplayDeathNotifier() { removeObitRecipient(this); }

    virtual void died() = 0;
    int  setListener(const sp<DisplayFeatureListener>& listener);
    static const sp<IDisplayFeatureControl> getDisplayFeatureService();

private:
    IDisplayDeathNotifier &operator=(const IDisplayDeathNotifier &);
    IDisplayDeathNotifier(const IDisplayDeathNotifier &);

    static void addObitRecipient(const wp<IDisplayDeathNotifier>& recipient);
    static void removeObitRecipient(const wp<IDisplayDeathNotifier>& recipient);

    class DeathNotifier: public IBinder::DeathRecipient
    {
    public:
                DeathNotifier() {}
        virtual ~DeathNotifier();

        virtual void binderDied(const wp<IBinder>& who);
    };

    friend class DeathNotifier;

    static  Mutex                                   sServiceLock;
    static  sp<IDisplayFeatureControl>                 sDisplayFeatureService;
    static  sp<DeathNotifier>                       sDeathNotifier;
    static  SortedVector< wp<IDisplayDeathNotifier> > sObitRecipients;
};


class  DisplayFeature: public virtual IDisplayDeathNotifier
{

public:

    DisplayFeature();
    ~DisplayFeature();

    int setFunctionEnable(int displayId, int caseId, int modeId, int cookie);
    int setFeatureEnable(int caseId, int modeId, int cookie);
    void sendBrightness(int modeId, int cookie);
    virtual void    notify(int msg, int ext1, int ext2, const Parcel *obj = NULL);
    void            died();
    void setListener(const sp<DisplayFeatureListener>& listener);
private:
    Mutex    mLock;
    sp<DisplayFeatureListener>     mListener;
};
};

#endif
