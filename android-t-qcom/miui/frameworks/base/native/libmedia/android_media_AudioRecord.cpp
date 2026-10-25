//
// Created by zengjing on 17-9-14.
//
//#define LOG_NDEBUG 0
#define LOG_TAG "MiuiAudioRecord_JNI"

#include <utils/Log.h>
#include <jni.h>
#include <android_runtime/AndroidRuntime.h>


#include <nativehelper/JNIHelp.h>
#include <nativehelper/JNIPlatformHelp.h>
#include "AudioLoopbackSource.h"

using namespace android;

// ----------------------------------------------------------------------------
static const char* const kClassPathName = "android/media/MiuiAudioRecord";
static const char* const kClassAudioInfoPathName = "android/media/MiuiAudioRecord$AudioInfo";
static const char* const kClassMetaDataPathName = "android/media/MiuiAudioRecord$MetaData";

// ----------------------------------------------------------------------------
struct fields_t {
    // these fields provide access from C++ to the...
    jclass    recordClass;              // MiuiAudioRecord java class global ref
    jfieldID  nativeRecordInJavaObj; // stores in Java the native MiuiAudioRecord object
};

static struct {
    jfieldID  fieldSize;            // AudioInfo.size
    jfieldID  fieldTimeUs;          // AudioInfo.timeUs
} javaAudioInfoFields;

static struct {
    jfieldID  fieldSample;            // MetaData.sampleRate
    jfieldID  fieldChannel;          // MetaData.channelCount
} javaMetaDataFields;

static fields_t javaAudioRecordFields;

static Mutex sLock;

static jboolean android_media_audio_AudioRecord_setup(
       JNIEnv* env, jobject thiz, jobject fileDescriptor, jlong size)
{
    Mutex::Autolock l(sLock);
    AudioLoopbackSource* audioSource = new AudioLoopbackSource(
        jniGetFDFromFileDescriptor(env, fileDescriptor), size);
    if (!audioSource->initCheck()) {
        ALOGE("AudioLoopbackSource init fail");
        env->SetLongField(thiz, javaAudioRecordFields.nativeRecordInJavaObj, 0);
        return JNI_FALSE;
    }
    env->SetLongField(thiz, javaAudioRecordFields.nativeRecordInJavaObj, (jlong)audioSource);
    return JNI_TRUE;
}

static jboolean android_media_audio_AudioRecord_start(
       JNIEnv* env, jobject thiz, jlong startTimeUs)
{
    Mutex::Autolock l(sLock);
    AudioLoopbackSource* audioSource = (AudioLoopbackSource*)env->GetLongField(
        thiz, javaAudioRecordFields.nativeRecordInJavaObj);
    if (audioSource == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException",
            "Unable to retrieve MiuiAudioRecord pointer for openFile()");
        return JNI_FALSE;
    }
    if (audioSource->start(startTimeUs) != OK) {
        ALOGE("AudioLoopbackSource start fail");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

static jboolean android_media_audio_AudioRecord_stop(
       JNIEnv* env, jobject thiz)
{
    AudioLoopbackSource* audioSource = (AudioLoopbackSource*)env->GetLongField(
        thiz, javaAudioRecordFields.nativeRecordInJavaObj);
    if (audioSource == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException",
            "Unable to retrieve MiuiAudioRecord pointer for openFile()");
        return JNI_FALSE;
    }
    if (audioSource->stop() != OK) {
        ALOGE("AudioLoopbackSource stop fail");
        return JNI_FALSE;
    }
    {
        Mutex::Autolock l(sLock);
        delete audioSource;
    }
    env->SetLongField(thiz, javaAudioRecordFields.nativeRecordInJavaObj, 0);
    ALOGV("android_media_audio_AudioRecord_stop() done");

    return JNI_TRUE;
}

static jboolean android_media_audio_AudioRecord_fillBuffer(
       JNIEnv* env, jobject thiz,jobject info, jint offset, jint size)
{
    Mutex::Autolock l(sLock);
    AudioLoopbackSource* audioSource = (AudioLoopbackSource*)env->GetLongField(
        thiz, javaAudioRecordFields.nativeRecordInJavaObj);
    if (audioSource == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException",
            "Unable to retrieve MiuiAudioRecord pointer for openFile()");
        return JNI_FALSE;
    }

    int64_t timeUs = 0;
    size_t length = 0;
    if (audioSource->fillBuffer(offset, size, &timeUs, &length) != OK) {
        ALOGE("fillBuffer failed!");
        return JNI_FALSE;
    }
    env->SetLongField(
            info, javaAudioInfoFields.fieldSize, length);
    env->SetLongField(
            info, javaAudioInfoFields.fieldTimeUs, timeUs);
    return JNI_TRUE;
}

static jboolean android_media_audio_AudioRecord_getMetaData(
       JNIEnv* env, jobject thiz, jobject reply)
{
    Mutex::Autolock l(sLock);
    AudioLoopbackSource* audioSource = (AudioLoopbackSource*)env->GetLongField(
        thiz, javaAudioRecordFields.nativeRecordInJavaObj);
    if (audioSource == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException",
            "Unable to retrieve MiuiAudioRecord pointer for openFile()");
        return JNI_FALSE;
    }

    int32_t sampleRate, channelCount;
    if (audioSource->getMetadata(&sampleRate, &channelCount) != OK) {
        return JNI_FALSE;
    }

    env->SetIntField(
            reply, javaMetaDataFields.fieldChannel, channelCount);
    env->SetIntField(
            reply, javaMetaDataFields.fieldSample, sampleRate);
    return JNI_TRUE;
}

static const JNINativeMethod gMethods[] = {
    { "native_setup", "(Ljava/io/FileDescriptor;J)Z", (void *)android_media_audio_AudioRecord_setup },
    { "native_start", "(J)Z", (void *)android_media_audio_AudioRecord_start },
    { "native_stop", "()Z", (void *)android_media_audio_AudioRecord_stop },
    { "native_fillBuffer", "(Landroid/media/MiuiAudioRecord$AudioInfo;II)Z", (void *)android_media_audio_AudioRecord_fillBuffer },
    { "native_getMetadata", "(Landroid/media/MiuiAudioRecord$MetaData;)Z", (void *)android_media_audio_AudioRecord_getMetaData },
};

#define JAVA_NATIVEMIUIAUDIORECORDINJAVAOBJ_FIELD_NAME "mNativeAudioRecordInJavaObj"

int register_android_media_MiuiAudioRecord(JNIEnv *env)
{
    javaAudioRecordFields.nativeRecordInJavaObj = NULL;

    // Get the JetPlayer java class
    jclass audioRecordClass = env->FindClass(kClassPathName);
    if (audioRecordClass == NULL) {
        ALOGE("Can't find %s", kClassPathName);
        return -1;
    }

    // Get the mNativeRecordInJavaObj variable field
    javaAudioRecordFields.nativeRecordInJavaObj = env->GetFieldID(
            audioRecordClass, JAVA_NATIVEMIUIAUDIORECORDINJAVAOBJ_FIELD_NAME, "J");
    if (javaAudioRecordFields.nativeRecordInJavaObj == NULL) {
        ALOGE("Can't find MiuiAudioRecord.%s", JAVA_NATIVEMIUIAUDIORECORDINJAVAOBJ_FIELD_NAME);
        return -1;
    }

    env->DeleteLocalRef(audioRecordClass);

    // Get the AudioInfo class and fields
    jclass audioInfoClass = env->FindClass(kClassAudioInfoPathName);
    if (audioInfoClass == NULL) {
        ALOGE("Can't find %s", kClassAudioInfoPathName);
        return -1;
    }

    javaAudioInfoFields.fieldSize = env->GetFieldID(
        audioInfoClass, "size", "J");
    if (javaAudioInfoFields.fieldSize == NULL) {
        ALOGE("Can't find AudioInfo.size");
        return -1;
    }

    javaAudioInfoFields.fieldTimeUs = env->GetFieldID(
        audioInfoClass, "timeUs", "J");
    if (javaAudioInfoFields.fieldTimeUs == NULL) {
        ALOGE("Can't find AudioInfo.timeUs");
        return -1;
    }
    env->DeleteLocalRef(audioInfoClass);

    // Get the MetaData class and fields
    jclass metaDataClass = env->FindClass(kClassMetaDataPathName);
    if (metaDataClass == NULL) {
        ALOGE("Can't find %s", kClassMetaDataPathName);;
        return -1;
    }

    javaMetaDataFields.fieldChannel = env->GetFieldID(
        metaDataClass, "channelCount", "I");
    if (javaMetaDataFields.fieldChannel == NULL) {
        ALOGE("Can't find MetaData.channelCount");
        return -1;
    }

    javaMetaDataFields.fieldSample = env->GetFieldID(
        metaDataClass, "sampleRate", "I");
    if (javaMetaDataFields.fieldSample == NULL) {
        ALOGE("Can't find MetaData.sampleRate");
        return -1;
    }
    env->DeleteLocalRef(metaDataClass);

    return AndroidRuntime::registerNativeMethods(env,
                kClassPathName, gMethods, NELEM(gMethods));
}
