#include <jni.h>
#include <string>
#include <utils/Log.h>
#include <stdio.h>
#include <vendor/xiaomi/hardware/miperf/1.0/IMiPerf.h>
#include <vendor_xiaomi_hardware_miperf_V1_0_MiPerf.h>

using namespace android;
using namespace std;
using vendor::xiaomi::hardware::miperf::V1_0::IMiPerf;
using vendor::xiaomi::hardware::miperf::V1_0::pack_list;
using ::android::hardware::hidl_death_recipient;
using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::Return;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::sp;
using ::android::wp;

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "MiPerf_JNI"
#endif

/* Switch the boostmode */
static string BOOSTMODE_PERFLOCK_ACQUIRE   = "PERFLOCK_ACQUIRE";
static string BOOSTMODE_PERFLOCK_RELEASE   = "PERFLOCK_RELEASE";
static string BOOSTMODE_WRITENODE_ACQUIRE  = "WRITENODE_ACQUIRE";
static string BOOSTMODE_WRITENODE_RECOVER  = "WRITENODE_RECOVER";
static string BOOSTMODE_MULTIPLE_XML       = "MULTIPLEMODE";

/* Represents miperf's HIDL service */
static sp<IMiPerf> gIService = nullptr;
static sp<hidl_death_recipient> mChd = nullptr;

class IMiperfDeathRecipient: public hidl_death_recipient {
    private:
        int mDeathCount = 1;
    public:
        void serviceDied(uint64_t cookie, const wp<IBase>& who) {
            ALOGE("miperf hidl services is died!");
            gIService = IMiPerf::getService();
            if (gIService != nullptr) {
                Return<bool> ret = gIService->linkToDeath(mChd, mDeathCount++);
                ALOGD("IMiperfDeathRecipient: Get IMiPerf@1.0 service success!\n");
            }
        }
};

/* Get the miperf's hidl service */
sp<IMiPerf> miperfServiceGet() {
    if (gIService == nullptr) {
        gIService = IMiPerf::getService();
        if (gIService == nullptr) {
            ALOGE("Get IMiPerf@1.0 service failed!\n");
        }else {
            ALOGD("Get IMiPerf@1.0 service success!\n");
            mChd = new IMiperfDeathRecipient();
            Return<bool> ret = gIService->linkToDeath(mChd, 0);
            if(!ret.isOk() || !ret) {
                ALOGE("IMiperfDeathRecipient: linkToDeath in failed!\n");
            }
        }
    }
    return gIService;
}

/*
 * Function: nativePerfAcqXml - Call the perflock mechanism
 * @pid: The pid number of the current application
 * @activityName: The current activity name
 * @packageName: The current package name
 * return: Return the handle num if success, -1 or < 0 if failed
 * */
JNIEXPORT jint JNICALL Java_android_os_MiPerf_nativePerfAcqXml(JNIEnv *env, jclass, jint pid, jstring activityName, jstring packageName, jstring boostMode) {
    gIService = miperfServiceGet();
    if(gIService == nullptr){
        ALOGD("Failed to get MiPerfService!\n");
        return -1;
    }
    const char* ActivityName;
    const char* PackageName;
    const char* BoostMode;
    ActivityName = env->GetStringUTFChars(activityName, JNI_FALSE);
    PackageName = env->GetStringUTFChars(packageName, JNI_FALSE);
    BoostMode = env->GetStringUTFChars(boostMode, JNI_FALSE);

    int ret = -1;
    if (BoostMode == BOOSTMODE_PERFLOCK_ACQUIRE) {
        ret = gIService->callPerfLock(pid, 1, PackageName, ActivityName, BOOSTMODE_PERFLOCK_ACQUIRE);
    } else if (BoostMode == BOOSTMODE_PERFLOCK_RELEASE) {
        ret = gIService->callPerfLock(pid, 1, PackageName, ActivityName, BOOSTMODE_PERFLOCK_RELEASE);
    } else if (BoostMode == BOOSTMODE_WRITENODE_ACQUIRE) {
        ret = gIService->callWriteToNode(pid, 1, PackageName, ActivityName, BOOSTMODE_WRITENODE_ACQUIRE);
    } else if (BoostMode == BOOSTMODE_WRITENODE_RECOVER) {
        ret = gIService->callWriteToNode(pid, 1, PackageName, ActivityName, BOOSTMODE_WRITENODE_RECOVER);
    }
    return ret;
}

/*
 * Function: nativeGetXmlHashMap - Get the Qcom's xml content
 * return: Return the HashMap of Qcom's xml
 * */
JNIEXPORT jobject JNICALL Java_android_os_MiPerf_nativeGetXmlHashMap(JNIEnv *env, jclass) {
    gIService = miperfServiceGet();

    hidl_vec<pack_list> vec_single;
    auto cb = [&vec_single](const hidl_vec<pack_list>& val) {
        vec_single = val;
    };

    jclass class_hashmap = env->FindClass("java/util/HashMap");
    jmethodID hashmap_init = env->GetMethodID(class_hashmap, "<init>","()V");
    jobject HashMap = env->NewObject(class_hashmap, hashmap_init, "");
    jmethodID HashMap_put = env->GetMethodID(class_hashmap, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    if(gIService == nullptr) {
        ALOGE("Failed to get MiPerfService!");
        return HashMap;
    }

    gIService->getXmlcont(cb);
    // ALOGD("The vecsize is %d\n", vec_single.size());
    for(int i=0; i<vec_single.size(); i++) {
        jmethodID hashmap_init_tmp = env->GetMethodID(class_hashmap, "<init>","()V");
        jobject hashmap_tmp = env->NewObject(class_hashmap, hashmap_init_tmp, "");
        env->CallObjectMethod(hashmap_tmp, HashMap_put, env->NewStringUTF(vec_single[i].actName.c_str()), env->NewStringUTF(vec_single[i].boostMode.c_str()));
        // ALOGD("packName: %s, actName: %s, boostMode: %s\n", vec_single[i].packName.c_str(), vec_single[i].actName.c_str(), vec_single[i].boostMode.c_str());
        env->CallObjectMethod(HashMap, HashMap_put, env->NewStringUTF(vec_single[i].packName.c_str()), hashmap_tmp);
    }
    return HashMap;
}

/*
 * Function: change config from cloud
 * return:  1 for success, 0 for failed
 * */
JNIEXPORT jint JNICALL Java_android_os_MiPerf_nativeUpdateConfig(JNIEnv *env, jclass) {
    gIService = miperfServiceGet();
    if(gIService == nullptr) {
        ALOGD("Failed to get MiPerfService!");
        return 0;
    }

    ALOGD("updateConfig jni called");
    gIService->updateConfig();
    return 1;
}
