#ifndef INCLUDE_COM_XIAOMI_NETWORKBOOST_UTILS
#define INCLUDE_COM_XIAOMI_NETWORKBOOST_UTILS

#include <jni.h>
#include <nativehelper/JNIHelp.h>
#include <string>
#include <vector>

typedef enum
{
    GET_FAIL = 0,
    GET_SUCCESS_NOATTACH,
    GET_SUCCESS_ATTACH,
} Get_JNIEnv_Status;

class AttachToJavaThread {
public:
    AttachToJavaThread();
    ~AttachToJavaThread();

    JNIEnv* getEnv() { return env_; }

private:
    bool selfAttached = false;
    JNIEnv* env_;
};

Get_JNIEnv_Status checkAndAttachThread(JNIEnv ** env);
void doDetachThread();
int jstring2string(JNIEnv* env, jstring origin, std::string &out);
int stringSplit(const std::string& str, const char split, std::vector<std::string>& res);

#endif // INCLUDE_COM_XIAOMI_NETWORKBOOST_UTILS