#ifndef OCTVM_SYSTEMKLOPROXY_H
#define OCTVM_SYSTEMKLOPROXY_H

#include <utils/threads.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <binder/BinderService.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include "klo_task.h"
#include "octvm/ISystemKloService.h"

namespace android {

//klo operation proxy
class SystemKloProxy : public BinderService<SystemKloProxy>, public BnSystemKloService
{
    friend class BinderService<SystemKloProxy>;

    class KloEventThread : public Thread {
    public:
        KloEventThread(const char* sourceName, int sourceType);
        virtual ~KloEventThread() {}

        virtual bool threadLoop();
        const char* getName(){ return mSourceName.string(); }
    private:
        int mSourceType;
        String8 mSourceName;
    };

public:
    SystemKloProxy();
    ~SystemKloProxy();

    static const char* getServiceName();

    static bool isServiceStarted();

    static bool isFeatureOn();

    static void setFeatureStatus(char *status);

    static void getFeatureStatus(char *status);

    int32_t klo_feature_enable(int enable);

    int32_t register_klo_task(klo_task_t *task);

    int32_t unregister_klo_task(int32_t task_id);

    int32_t set_klo_dump_files(const Vector<String16>& fileDescs);

    int32_t klo_get_anonymous_id(String16& anonymous_id);

    int32_t klo_dump_system(const char *file_path);

    int32_t klo_system_debug(const Vector<String16>& args);

    int32_t dump(int fd, const Vector<String16>& args);

protected:
    void addDefaultMonitor();
    void startKloMonitor();
    void stopKloMonitor();

    int32_t FeatureEnable(bool enable);

public:
    static const String8 SName;
    static const String8 FeaturePropKey;

private:
    KeyedVector<int, sp<KloEventThread> > mEventsMonitor;
};

};

#endif
