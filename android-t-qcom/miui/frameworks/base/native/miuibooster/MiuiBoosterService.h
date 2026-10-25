/*
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#ifndef ANDROID_PERFSERVICE_H
#define ANDROID_PERFSERVICE_H

#include "IMiuiBoosterManager.h"
#include <binder/BinderService.h>
#include "miuibooster.h"

namespace android {

class MiuiBoosterService :
    public BinderService<MiuiBoosterService>,
    public BnMiuiBoosterManager
{
public:
    MiuiBoosterService();
    bool is_uid_authorized(int uid);
    int internal_pre_check(int uid);
    virtual int removeAuthorizedUid(int uid);
    virtual int checkPermission(String16& pkg_name, int uid);
    virtual int checkPermission_key(String16& pkg_name, int uid, String16& auth_key);
    virtual int requestCpuHighFreq(int uid, int level, int timeoutms);
    virtual int cancelCpuHighFreq(int uid);
    virtual int requestGpuHighFreq(int uid, int level, int timeoutms);
    virtual int cancelGpuHighFreq(int uid);
    virtual int requestIOHighFreq(int uid, int level, int timeoutms);
    virtual int cancelIOHighFreq(int uid);
    virtual int requestDdrHighFreq(int uid, int level, int timeoutms);
    virtual int cancelDdrHighFreq(int uid);
    virtual int requestVpuHighFreq(int uid, int level, int timeoutms) ;
    virtual int cancelVpuHighFreq(int uid);
    virtual int requestMemory(int uid, int level, int timeoutms);
    virtual int cancelMemory(int uid);
    virtual int requestNetwork(int uid, int level, int timeoutms);
    virtual int cancelNetwork(int uid);
    virtual int requestThreadPriority(int uid, int req_tid, int timeoutms);
    virtual int cancelThreadPriority(int uid, int req_tid);
    virtual int requestIOPrefetch(int uid, String16& filePath);
    virtual int requestBindCore(int uid, int tid);
    virtual int requestSetScene(int uid, String16& pkgName, int sceneId);

private:
    MiuiBoosterUtils miui_booster;

    set<int> authorized_uids;
};

} // namespace android

#endif // ANDROID_PERFSERVICE_H
