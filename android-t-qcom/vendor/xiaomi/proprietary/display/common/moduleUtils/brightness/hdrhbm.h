#ifndef _HDR_HBM_H_
#define _HDR_HBM_H_
#include <unistd.h>
#include <pthread.h>
#include <utils/StrongPointer.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/poll.h> 
#include <utils/RefBase.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <utils/Log.h>
#include "hdr_panel_hbm_mode_interface.h"

class HDRHBMImpl:public PanelHBMModeInterface
{
 public:
    HDRHBMImpl();
    ~HDRHBMImpl();
    static HDRHBMImpl* CreateHDRHBMImpl();
    static void DestroyHDRHBMImpl();

    bool IsHBMMode();
    float GetNitsforHBM();

 private:
    static HDRHBMImpl* mHDRHBMImpl_;
    static constexpr const char* kDispFeaturePath = "/dev/mi_display/disp_feature";
    static void *ThreadWrapperBacklight(void *);
    int threadFuncBacklight();
    int registerPollEvent();
    bool mIsHBM = false;
    float mCurNits = 0.0f;
    pthread_t mThreadBacklight;
    pollfd mDFPollFd;
};

class PanelHBMModeFactoryImp : public PanelHBMModeFactoryIntf {
 public:
    virtual ~PanelHBMModeFactoryImp() {}
    PanelHBMModeInterface* CreatePanelHBMModeIntf(const char*);
    void DestroyPanelHBMModeIntf(PanelHBMModeInterface*);
};
#endif
