#include <jni.h>
#include <string>
#include <android/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#define LOG_TAG "native_pegame"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
using namespace std;

extern "C" JNIEXPORT jstring JNICALL Java_com_miui_powerkeeper_perfengine_PeGameController_nativeParseDevice(JNIEnv * env, jobject jobj, jstring deviceName) {
    std::vector<std::string> qcom855Vector;
    qcom855Vector.push_back("nabu");

    std::vector<std::string> qcom865Vector;
    qcom865Vector.push_back("cmi");
    qcom865Vector.push_back("umi");
    qcom865Vector.push_back("lmi");
    qcom865Vector.push_back("lmipro");
    qcom865Vector.push_back("cas");
    qcom865Vector.push_back("apollo");
    qcom865Vector.push_back("alioth");
    qcom865Vector.push_back("aliothin");
    qcom865Vector.push_back("elish");
    qcom865Vector.push_back("enuma");
    qcom865Vector.push_back("psyche");
    qcom865Vector.push_back("poussin"); //L10A
    qcom865Vector.push_back("munch");
    qcom865Vector.push_back("dagu");//L81A

    std::vector<std::string> qcom888Vector;
    qcom888Vector.push_back("venus");
    qcom888Vector.push_back("haydn");
    qcom888Vector.push_back("star");
    qcom888Vector.push_back("mars");
    qcom888Vector.push_back("cetus");
    qcom888Vector.push_back("odin");
    qcom888Vector.push_back("vili");

    std::vector<std::string> qcom8450Vector;
    qcom8450Vector.push_back("zeus");
    qcom8450Vector.push_back("cupid");
    qcom8450Vector.push_back("ingres");
    qcom8450Vector.push_back("zizhan");
    qcom8450Vector.push_back("thor");
    qcom8450Vector.push_back("unicorn");
    qcom8450Vector.push_back("diting");
    qcom8450Vector.push_back("ditingp");
    qcom8450Vector.push_back("mayfly");
    qcom8450Vector.push_back("mondrian");
    qcom8450Vector.push_back("marble");

    std::vector<std::string> qcom8550Vector;
    qcom8550Vector.push_back("nuwa");
    qcom8550Vector.push_back("fuxi");
    qcom8550Vector.push_back("socrates");

    std::vector<std::string> qcom7325Vector;
    qcom7325Vector.push_back("mona");
    qcom7325Vector.push_back("zijin");
    qcom7325Vector.push_back("lisa");
    qcom7325Vector.push_back("ziyi");//L9S

    std::vector<std::string> qcom7X5Vector;
    qcom7X5Vector.push_back("gauguinpro");
    qcom7X5Vector.push_back("gauguin");
    qcom7X5Vector.push_back("renoir");
    qcom7X5Vector.push_back("veux");
    qcom7X5Vector.push_back("vangogh");
    qcom7X5Vector.push_back("picasso");

    std::vector<std::string> qcom4375Vector;
    qcom4375Vector.push_back("sunstone");


    std::vector<std::string> MTKAVector;
    MTKAVector.push_back("bomb");
    MTKAVector.push_back("atom");
    MTKAVector.push_back("cezanne");
    MTKAVector.push_back("cannon");
    MTKAVector.push_back("camellia");
    MTKAVector.push_back("chopin");
    MTKAVector.push_back("pissarro");
    MTKAVector.push_back("pissarropro");
    MTKAVector.push_back("pissarroin");
    MTKAVector.push_back("pissarroinpro");

    std::vector<std::string> MTKBVector;
    MTKBVector.push_back("ares");
    MTKBVector.push_back("agate");
    MTKBVector.push_back("evergo");
    MTKBVector.push_back("light");

    std::vector<std::string> MTKCVector;
    MTKCVector.push_back("matisse");
    MTKCVector.push_back("rubens");
    MTKCVector.push_back("xaga");
    MTKCVector.push_back("xagapro");
    MTKCVector.push_back("plato");
    MTKCVector.push_back("daumier");
    MTKCVector.push_back("xagain");
    MTKCVector.push_back("xagaproin");

    std::vector<std::string> MTKDVector;
    MTKDVector.push_back("viva");
    MTKDVector.push_back("vida");

    const char * ret = "";
    const char * c_devicename = env-> GetStringUTFChars(deviceName, NULL);
    LOGI("nativeParseDevice_pegame %s", c_devicename);

    if (std::count(qcom855Vector.begin(), qcom855Vector.end(), c_devicename) > 0) {
        ret = "855";
    } else if (std::count(qcom865Vector.begin(), qcom865Vector.end(), c_devicename) > 0) {
        ret = "865";
    } else if (std::count(qcom888Vector.begin(), qcom888Vector.end(), c_devicename) > 0) {
        ret = "888";
    } else if (std::count(qcom8450Vector.begin(), qcom8450Vector.end(), c_devicename) > 0) {
        ret = "8450";
    } else if (std::count(qcom8550Vector.begin(), qcom8550Vector.end(), c_devicename) > 0) {
        ret = "8550";
    } else if (std::count(qcom7325Vector.begin(), qcom7325Vector.end(), c_devicename) > 0) {
        ret = "7325";
    } else if (std::count(qcom7X5Vector.begin(), qcom7X5Vector.end(), c_devicename) > 0) {
        ret = "7X5";
    } else if (std::count(qcom4375Vector.begin(), qcom4375Vector.end(), c_devicename) > 0) {
        ret = "4375";
    } else if (std::count(MTKAVector.begin(), MTKAVector.end(), c_devicename) > 0) {
        ret = "mtkA";
    } else if (std::count(MTKBVector.begin(), MTKBVector.end(), c_devicename) > 0) {
        ret = "mtkB";
    } else if (std::count(MTKCVector.begin(), MTKCVector.end(), c_devicename) > 0) {
        ret = "mtkC";
    } else if (std::count(MTKDVector.begin(), MTKDVector.end(), c_devicename) > 0) {
        ret = "mtkD";
    }

    env->ReleaseStringUTFChars(deviceName,c_devicename);
    return env->NewStringUTF(ret);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_miui_powerkeeper_perfengine_PeGameController_nativeIsPowerModeDevice(JNIEnv * env, jobject jobj, jstring deviceName) {
    std::vector<std::string> powerModeEnbleVector;
    powerModeEnbleVector.push_back("venus");

    jboolean ret = JNI_FALSE;
    const char * c_devicename = env-> GetStringUTFChars(deviceName, NULL);
    LOGI("nativeIsPowerModeDevice_pegame %s", c_devicename);

    if (std::count(powerModeEnbleVector.begin(), powerModeEnbleVector.end(), c_devicename) > 0) {
        ret = JNI_TRUE;
    } else {
        ret = JNI_FALSE;
    }
    env->ReleaseStringUTFChars(deviceName,c_devicename);
    return ret;
}

extern "C" JNIEXPORT jstring JNICALL Java_com_miui_powerkeeper_perfengine_MtkBoost_nativeGetCMD(JNIEnv * env, jobject jobj, jstring deviceName) {
    std::vector<std::string> CMDADevicesVector;
    CMDADevicesVector.push_back("atom");
    CMDADevicesVector.push_back("bomb");
    CMDADevicesVector.push_back("cezanne");
    CMDADevicesVector.push_back("cannon");
    CMDADevicesVector.push_back("chopin");
    CMDADevicesVector.push_back("camellia");
    CMDADevicesVector.push_back("pissarro");
    CMDADevicesVector.push_back("pissarropro");
    CMDADevicesVector.push_back("pissarroin");
    CMDADevicesVector.push_back("pissarroinpro");

    std::vector<std::string> CMDBDevicesVector;
    CMDBDevicesVector.push_back("ares");
    CMDBDevicesVector.push_back("agate");
    CMDBDevicesVector.push_back("evergo");
    CMDBDevicesVector.push_back("light");

    std::vector<std::string> CMDCDevicesVector;
    CMDCDevicesVector.push_back("matisse");
    CMDCDevicesVector.push_back("rubens");
    CMDCDevicesVector.push_back("xaga");
    CMDCDevicesVector.push_back("xagapro");
    CMDCDevicesVector.push_back("plato");
    CMDCDevicesVector.push_back("daumier");
    CMDCDevicesVector.push_back("xagain");
    CMDCDevicesVector.push_back("xagaproin");


    std::vector<std::string> CMDDDevicesVector;
    CMDDDevicesVector.push_back("viva");
    CMDDDevicesVector.push_back("vida");

    const char * ret = "";
    const char * c_devicename = env-> GetStringUTFChars(deviceName, NULL);
    LOGI("nativeGetCMD_pegame %s", c_devicename);

    if (std::count(CMDADevicesVector.begin(), CMDADevicesVector.end(), c_devicename) > 0) {
        // CMDA
        ret = "perflock#0x01000000_0_0x01408300_100_0x0201c000_60_0x0201c100_60_0x02020000_15_0x01410000_1_0x00410000_1_0x02028000_1_0x01424100_0_0x01424200_1_0x03400000_2_0x01424000_40000000_0x03000000_8_0x02c14000_1_0x0141c000_1#3600";
    } else if (std::count(CMDBDevicesVector.begin(), CMDBDevicesVector.end(), c_devicename) > 0) {
        // CMDB
        ret = "perflock#0x01000000_0_0x01408300_100_0x0201c000_60_0x0201c100_60_0x02020000_15_0x01410000_1_0x00410000_1_0x02028000_1_0x01424100_0_0x01424200_1_0x03400000_2_0x01424000_40000000_0x03000000_8_0x02c14000_1_0x02038000_0#3600";
    } else if (std::count(CMDCDevicesVector.begin(), CMDCDevicesVector.end(), c_devicename) > 0) {
        // CMDC
        ret = "perflock#0x01000000_0_0x01408300_100_0x01438400_0_0x01438500_0_0x01438600_40000_0x01438700_40000_0x01438800_40000_0x01010200_1_0x0143c100_1_0x00c00000_5#3600";
    } else if (std::count(CMDDDevicesVector.begin(), CMDDDevicesVector.end(), c_devicename) > 0) {
        // CMDD
        ret = "perflock#0x01000000_0_0x01408300_100_0x0201c000_120_0x0201c100_120_0x02020000_15_0x01410000_1_0x00410000_1_0x01424100_0_0x01424200_1_0x01424000_40000000_0x03000000_8_0x03400000_3_0x02038000_0#3600";
    }
    env->ReleaseStringUTFChars(deviceName,c_devicename);
    return env->NewStringUTF(ret);
}

extern "C" JNIEXPORT jstring JNICALL Java_com_miui_powerkeeper_perfengine_PeGameController_nativeGetAt(JNIEnv * env, jobject jobj, jstring key) {
    const char * ret = "";
    const char * c_key = env-> GetStringUTFChars(key, NULL);
    LOGI("nativeGetAtt key: %s", c_key);

    if (strcmp(c_key, "A") == 0) {
        ret = "com.antutu.ABenchMark";
    } else {
        ret = "com.antutu.benchmark.full";
    }
    env->ReleaseStringUTFChars(key,c_key);
    return env->NewStringUTF(ret);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_miui_powerkeeper_PowerKeeperManager_nativeIsGestureBoostDevice(JNIEnv * env, jobject jobj, jstring deviceName) {
    std::vector<std::string> gestureBoostVector;
    gestureBoostVector.push_back("zeus");
    gestureBoostVector.push_back("cupid");
    gestureBoostVector.push_back("ingres");
    gestureBoostVector.push_back("zizhan");
    gestureBoostVector.push_back("thor");
    gestureBoostVector.push_back("loki");
    gestureBoostVector.push_back("diting");
    gestureBoostVector.push_back("ditingp");
    gestureBoostVector.push_back("mayfly");
    gestureBoostVector.push_back("nuwa");
    gestureBoostVector.push_back("fuxi");
    gestureBoostVector.push_back("mondrian");
    gestureBoostVector.push_back("socrates");
    gestureBoostVector.push_back("unicorn");
    gestureBoostVector.push_back("sunstone");

    jboolean ret = JNI_FALSE;
    const char * c_devicename = env-> GetStringUTFChars(deviceName, NULL);
    LOGI("nativeIsGestureBoostDevice: %s", c_devicename);

    if (std::count(gestureBoostVector.begin(), gestureBoostVector.end(), c_devicename) > 0) {
        ret = JNI_TRUE;
    } else {
        ret = JNI_FALSE;
    }
    env->ReleaseStringUTFChars(deviceName,c_devicename);
    return ret;
}
