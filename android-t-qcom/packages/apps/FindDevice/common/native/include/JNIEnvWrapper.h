#ifndef ANDROID_JNIENVWRAPPER_H
#define ANDROID_JNIENVWRAPPER_H

#include "xassert.h"
#include "util_common.h"

#include <jni.h>
#include <stdlib.h>
#include <new>

#define EXCEPTION_RETURN(r) if ( env.ExceptionCheck() ) { return r; }

// Wraps raw JNIEnv to ensure:
// 1. Methods that only throw when there's a bug won't throw and die
//    in a bug situation.
// 2. Ensure the return value is reasonable if no exception threw.
//    Or else, we just die.
// 3. Zero-length C arrays are always returned as non-NULL pointers.

// The reason:
// For 1. This simplifies the JNIEnv callers' code to not bother checking
//        exceptions on every call. (Note that checking what a method throws
//        is complex, because that requires a FindClass() call which may throw
//        an exception itself. )
//
// For 2. This fixes some JNIEnv document mist, such as that some versions of
//        document say GetByteArrayElements() throws OOM exceptions whilst
//        others say it doesn't.
//
// For 3. This also simplifies the JNIEnv callers' code. The return value can
//        be used directly to call functions requiring a non-NULL pointer even
//        the length of the array is zero. Such as memcpy().
//

class JNIEnvWrapper {

public:
    inline
    JNIEnvWrapper(JNIEnv * pRawEnv) {
        mpEnv = pRawEnv;
    }

    inline
    operator JNIEnv * () {
       return mpEnv;
    }

private:
    inline
    void dieOnException() {
        if (mpEnv->ExceptionCheck()) {
            xassert::die("Unexpected exception. ");
        }
    }

    template<class T, class RE, class TE>
    inline
    T ensureResult(T rtn, RE rtnExpectation, TE throwException) {
        if (!throwException(mpEnv)) {
            xassert::die("Unexpected exception. ");
        }
        if ( !rtnExpectation(rtn) && !mpEnv->ExceptionCheck() ) {
            xassert::die("Unexpected return value with no exception. ");
        }
        return rtn;
    }

    template <class T>
    inline static
    bool RE_NOT_NEGTIVE(T v) { return v >= 0; }

    template <class T>
    inline static
    bool RE_NON_NULL(T v) { return v; }

    inline static
    bool RE_NO_EXPECTATION(void *) { return true; }

    inline static
    bool TE_NO_THROW(JNIEnv * pRawEnv) { return !pRawEnv->ExceptionCheck(); }

    inline static
    bool TE_THROW(JNIEnv *) { return true; }

public:

    // NO THROW.
    inline
    jsize GetArrayLength(jarray array) {
        dieOnException();
        return ensureResult(mpEnv->GetArrayLength(array), RE_NOT_NEGTIVE<jsize>, TE_NO_THROW);
    }

    // NO THROW.
    inline
    void SetBooleanArrayRegion(jbooleanArray array, jsize start, jsize len, const jboolean* buf) {
        dieOnException();
        mpEnv->SetBooleanArrayRegion(array, start, len, buf);
        ensureResult((void *)NULL, RE_NO_EXPECTATION, TE_NO_THROW);
    }
/*
    // THROW.
    inline
    const char* GetStringUTFChars(jstring string, jboolean* isCopy) {
        dieOnException();
        return ensureResult(mpEnv->GetStringUTFChars(string, isCopy), RE_NON_NULL<const char *>, TE_THROW);
    }
*/

/*
    // NO THROW
    inline
    void ReleaseStringUTFChars(jstring string, const char* utf) {
        dieOnException();
        mpEnv->ReleaseStringUTFChars(string, utf);
        ensureResult((void *)NULL, RE_NO_EXPECTATION, TE_NO_THROW);
    }
*/

    // THROW
    inline
    jobjectArray NewObjectArray(jsize length, jclass elementClass, jobject initialElement) {
        dieOnException();
        return ensureResult(mpEnv->NewObjectArray(length, elementClass, initialElement), RE_NON_NULL<jobjectArray>, TE_THROW);
    }

    // THROW
    inline
    jclass FindClass(const char* name) {
        dieOnException();
        return ensureResult(mpEnv->FindClass(name), RE_NON_NULL<jclass>, TE_THROW);
    }

    // NO THROW
    inline
    void SetObjectArrayElement(jobjectArray array, jsize index, jobject value) {
        dieOnException();
        mpEnv->SetObjectArrayElement(array, index, value);
        ensureResult((void *)NULL, RE_NO_EXPECTATION, TE_NO_THROW);
    }

    // THROW
    inline
    jstring NewStringUTF(const char* bytes) {
        dieOnException();
        return ensureResult(mpEnv->NewStringUTF(bytes), RE_NON_NULL<jstring>, TE_THROW);
    }

/*
    // THROW
    inline
    jbyte* GetByteArrayElements(jbyteArray array, jboolean* isCopy) {
        dieOnException();
        static jbyte * fake = NULL;
        if (fake == NULL) {
            fake = new jbyte [0];
        }
        if ( !GetArrayLength(array) ) {
            return fake;
        }
        return ensureResult(mpEnv->GetByteArrayElements(array, isCopy), RE_NON_NULL<jbyte*>, TE_THROW);
    }
*/

    // NO THROW
    inline
    void GetByteArrayRegion(jbyteArray array, jsize start, jsize len, jbyte *buf) {
        dieOnException();
        mpEnv->GetByteArrayRegion(array, start, len, buf);
        ensureResult((void *)NULL, RE_NO_EXPECTATION, TE_NO_THROW);
    }

/*
    // NO THROW
    inline
    void ReleaseByteArrayElements(jbyteArray array, jbyte* elems, jint mode) {
        dieOnException();
        mpEnv->ReleaseByteArrayElements(array, elems, mode);
        ensureResult((void *)NULL, RE_NO_EXPECTATION, TE_NO_THROW);
    }
*/

    // THROW
    inline
    jbyteArray NewByteArray(jsize length) {
        dieOnException();
        return ensureResult(mpEnv->NewByteArray(length), RE_NON_NULL<jbyteArray>, TE_THROW);
    }

    // NO THROW
    inline
    void SetByteArrayRegion(jbyteArray array, jsize start, jsize len, const jbyte* buf) {
        dieOnException();
        mpEnv->SetByteArrayRegion(array, start, len, buf);
        ensureResult((void *)NULL, RE_NO_EXPECTATION, TE_NO_THROW);
    }

    inline
    jboolean ExceptionCheck() {
        return mpEnv->ExceptionCheck();
    }

// Helpers
public:
    // NO THROW
    inline
    void getStringUTF(jstring j_str, std::vector<char> * pStr) {
        dieOnException();

        int j_strLen = mpEnv->GetStringLength(j_str);
        int strLen = mpEnv->GetStringUTFLength(j_str);

        pStr->clear();
        pStr->resize(strLen + 1, 0);
        mpEnv->GetStringUTFRegion(j_str, 0, j_strLen, addr(*pStr));
        ensureResult((void *)NULL, RE_NO_EXPECTATION, TE_NO_THROW);
    }

private:
    JNIEnv * mpEnv;
};

#endif //ANDROID_JNIENVWRAPPER_H
