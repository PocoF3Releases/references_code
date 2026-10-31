/*
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include "MiuiBoosterService.h"
#include <openssl/md5.h>

namespace android {

    MiuiBoosterService::MiuiBoosterService() {
        pwrn("MiuiBoosterService constructor start");
    }

    bool MiuiBoosterService::is_uid_authorized(int uid) {
        int len = 0;
        char value[PROP_VALUE_MAX];
        char* nexttok = nullptr;
        char* uid_str = nullptr;

        len = __system_property_get("persist.sys.mibridge_auth_uids", value);

        if (len <= 0)
            return false;

        nexttok = value;
        while ((uid_str = strsep(&nexttok, ","))) {
            pwrn("is_uid_authorized uid: %d, auth_uid_str:%s", uid, uid_str);
            if (atoi(uid_str) == (int)uid)
                return true;
        }

        return false;
    }

    int MiuiBoosterService::internal_pre_check(int uid) {

        int callingUid = IPCThreadState::self()->getCallingUid();

        if (callingUid != uid) {
            pwrn("Uid is not calling uid!!");
            return -3;
        }

        if (authorized_uids.find(uid) == authorized_uids.end()) {
            pwrn("Permission not granted!!");
            return -2;
        }

        return 0;
    }

    int MiuiBoosterService::removeAuthorizedUid(int uid) {
        int ret = authorized_uids.erase(uid);
        return ret;
    }

    int MiuiBoosterService::checkPermission(String16& pkg_name, int uid) {
        String8 s = (pkg_name) ? String8(pkg_name) : String8("");
        pwrn("MiuiBoosterService checkPermission: %s, %d", s.string(), uid);

        if (is_uid_authorized(uid)) {
            authorized_uids.insert(uid);
            return 1;
        }
        else
            return 0;
    }

    int MiuiBoosterService::checkPermission_key(String16& pkg_name, int uid, String16& auth_key) {
        String8 s = (pkg_name) ? String8(pkg_name) : String8("");
        String8 s2 = (auth_key) ? String8(auth_key) : String8("");
        pwrn("MiuiBoosterService checkPermission_key: %s, %d, %s", s.string(), uid, s2.string());

        //MD5 check
        MD5_CTX m;
        MD5_Init(&m);
        MD5_Update(&m, s.string(), s.size());
        //Add secret
        char buf_secret[] = "bJXMyDrJyXGSnJFgS";
        MD5_Update(&m, buf_secret, strlen(buf_secret));
        char buf_secret_more[] = "n1J2X7n3Y3hZgYj";
        MD5_Update(&m, buf_secret_more, strlen(buf_secret_more));

        uint8_t key[16];
        MD5_Final(key, &m);

        String8 str_check;

        for (int i = 0; i<16; i++) {
            str_check.appendFormat("%02X", key[i]);
        }

        pwrn("MiuiBoosterService checkPermission_key: out: %s", str_check.string());

        if (str_check == s2) {
            pwrn(" auth_key: correct!!!");
            authorized_uids.insert(uid);
            return 1;
        }
        else
            return 0;
    }

    int MiuiBoosterService::requestCpuHighFreq(int uid, int level, int timeoutms) {
        pwrn("MiuiBoosterService requestCpuHighFreq: %d, %d, %d", uid, level, timeoutms);

        //Unused
        int scene = 0;
        int64_t action = 0;
        int tid = 0;
        int64_t timestamp = 0;

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return miui_booster.requestCpuHighFreq(scene, action, level, timeoutms, tid, timestamp);
    }

    int MiuiBoosterService::cancelCpuHighFreq(int uid) {
        pdbg("MiuiBoosterService cancelCpuHighFreq: %d", uid);
        //Unused
        int tid = 0;
        int64_t timestamp = 0;

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return miui_booster.cancelCpuHighFreq(tid,timestamp);
    }

    int MiuiBoosterService::requestGpuHighFreq(int uid, int level, int timeoutms) {
        pwrn("MiuiBoosterService requestGpuHighFreq: %d, %d, %d", uid, level, timeoutms);

        //Unused
        int scene = 0;
        int64_t action = 0;
        int tid = 0;
        int64_t timestamp = 0;

        int pre_check = internal_pre_check(uid);
        pwrn("pre_check:%d",pre_check);

        if (pre_check < 0)
            return pre_check;

        return miui_booster.requestGpuHighFreq(scene, action, level, timeoutms, tid, timestamp);
    }

    int MiuiBoosterService::cancelGpuHighFreq(int uid) {
        pdbg("MiuiBoosterService cancelGpuHighFreq: %d", uid);

        //Unused
        int tid = 0;
        int64_t timestamp = 0;

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return miui_booster.cancelGpuHighFreq(tid, timestamp);
    }

    int MiuiBoosterService::requestIOHighFreq(int uid, int level, int timeoutms) {
        pwrn("MiuiBoosterService requestIOHighFreq: %d, %d, %d", uid, level, timeoutms);

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return -1;
    }

    int MiuiBoosterService::requestDdrHighFreq(int uid, int level, int timeoutms) {
        pwrn("MiuiBoosterService requestDdrHighFreq: %d, %d, %d", uid, level, timeoutms);

        //Unused
        int scene = 0;
        int64_t action = 0;
        int tid = 0;
        int64_t timestamp = 0;

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return miui_booster.requestDDRHighFreq(scene, action, level, timeoutms, tid, timestamp);
    }

    int MiuiBoosterService::cancelDdrHighFreq(int uid) {
        pwrn("MiuiBoosterService cancelDdrHighFreq: %d", uid);

        //Unused
        int tid = 0;
        int64_t timestamp = 0;

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return miui_booster.cancelDDRHighFreq(tid,timestamp);
    }

    int MiuiBoosterService::cancelIOHighFreq(int uid) {
        pdbg("MiuiBoosterService cancelIOHighFreq: %d", uid);

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return -1;
    }

    int MiuiBoosterService::requestIOPrefetch(int uid, String16& filePath) {
        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return miui_booster.requestIOPrefetch(filePath);
    }

    int MiuiBoosterService::requestBindCore(int uid, int tid) {
        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return miui_booster.requestBindCore(uid, tid);
    }

    int MiuiBoosterService::requestVpuHighFreq(int uid, int level, int timeoutms) {
        pdbg("MiuiBoosterService requestVpuHighFreq: %d, %d, %d", uid, level, timeoutms);

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return -1;
    }

    int MiuiBoosterService::cancelVpuHighFreq(int uid) {
        pdbg("MiuiBoosterService cancelVpuHighFreq: %d", uid);

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return -1;
    }

    int MiuiBoosterService::requestMemory(int uid, int level, int timeoutms) {
        pdbg("MiuiBoosterService requestMemory: %d, %d, %d", uid, level, timeoutms);

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return -1;
    }

    int MiuiBoosterService::cancelMemory(int uid) {
        pdbg("MiuiBoosterService cancelMemory: %d", uid);

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return -1;
    }

    int MiuiBoosterService::requestNetwork(int uid, int level, int timeoutms) {
        pdbg("MiuiBoosterService requestNetwork: %d, %d, %d", uid, level, timeoutms);

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return -1;
    }

    int MiuiBoosterService::cancelNetwork(int uid) {
        pdbg("MiuiBoosterService cancelNetwork: %d", uid);

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        return -1;
    }

    int MiuiBoosterService::requestThreadPriority(int uid, int req_tid, int timeoutms) {
        pwrn("MiuiBoosterService requestThreadPriority: %d, %d, %d", uid, req_tid, timeoutms);
        //Unused
        int scene = 0;
        int64_t action = 0;
        int callertid = 0;
        int64_t timestamp = 0;

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        vector<int> bindtids;
        bindtids.push_back(req_tid);

        return miui_booster.requestThreadPriority(scene, action, bindtids, timeoutms, callertid, timestamp);
    }

    int MiuiBoosterService::cancelThreadPriority(int uid, int req_tid) {
        pdbg("MiuiBoosterService cancelThreadPriority: %d, %d", uid, req_tid);
        //Unused
        int callertid = 0;
        int64_t timestamp = 0;

        int pre_check = internal_pre_check(uid);

        if (pre_check < 0)
            return pre_check;

        vector<int> bindtids;
        bindtids.push_back(req_tid);

        return miui_booster.cancelThreadPriority(bindtids, callertid, timestamp);
    }

    int MiuiBoosterService::requestSetScene(int uid, String16& pkgName, int sceneId) {
        //String8 s = (pkgName) ? String8(pkgName) : String8("");
        //pwrn("MiuiBoosterService requestSetScene: %d, %s, %d", uid, s.string(), sceneId);

        int pre_check = internal_pre_check(uid);
        if (pre_check < 0)
            return pre_check;

        return miui_booster.requestSetScene(pkgName, sceneId);
    }
};
