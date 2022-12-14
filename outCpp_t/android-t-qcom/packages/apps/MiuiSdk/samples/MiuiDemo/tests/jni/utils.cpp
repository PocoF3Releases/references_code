
#include "utils.h"
#include <pthread.h>
#include <stdlib.h>
#include <utils/Log.h>

template<const int MAX_LOCAL = 5>
class LocalEnv {
    JNIEnv *env;
    jobject locals[MAX_LOCAL];
    int     localSize;

    template<typename T>
    T addRef(T obj) {
        if (obj != NULL && localSize < MAX_LOCAL) {
            locals[localSize++] = (jobject)obj;
        }
        return obj;
    }

    LocalEnv(): env(env), localSize(0) { }

public:
    jstring newStringUTF(const char* str) {
        return addRef(env->NewStringUTF(str));
    }

    LocalEnv(JNIEnv* env) : env(env), localSize(0) {
    }

    ~LocalEnv() {
        for (int i = 0; i < localSize; i++) {
            if (locals[i] != NULL) {
                env->DeleteLocalRef(locals[i]);
            }
        }
    }

    JNIEnv* operator->() { return env; }

};

#define _TO_JVALUE(type) template<const int MAX_LOCAL> inline j##type toJvalue(LocalEnv<MAX_LOCAL>& env, type t) { return (j##type)t; }
_TO_JVALUE(int)
_TO_JVALUE(long)
_TO_JVALUE(short)
_TO_JVALUE(double)
_TO_JVALUE(float)
#undef _TO_JVALUE
template<const int MAX_LOCAL>
inline jboolean toJvalue(LocalEnv<MAX_LOCAL>& env, bool t) { return (jboolean)t;}
template<const int MAX_LOCAL>
inline jstring toJvalue(LocalEnv<MAX_LOCAL>& env, const char* str) {
    return str ? env.newStringUTF(str) : NULL;
}


struct JavaAssert {

    jclass clazz;
    jmethodID assertEqualsLong;
    //jmethodID assertEqualsInt;
    //jmethodID assertEqualsShort;
    jmethodID assertEqualsDouble;
    //jmethodID assertEqualsFloat;
    //jmethodID assertEqualsBoolean;
    //jmethodID assertEqualsString;
    jmethodID assertTrue;
    jmethodID assertFalse;


    void init(JNIEnv *env) {
        clazz = env->FindClass("org/junit/Assert");
        clazz = (jclass)env->NewGlobalRef(clazz);

        assertEqualsLong    = env->GetStaticMethodID(clazz, "assertEquals", "(Ljava/lang/String;JJ)V");
        //assertEqualsInt     = env->GetStaticMethodID(clazz, "assertEquals", "(Ljava/lang/String;II)V");
        //assertEqualsShort   = env->GetStaticMethodID(clazz, "assertEquals", "(Ljava/lang/String;SS)V");
        assertEqualsDouble  = env->GetStaticMethodID(clazz, "assertEquals", "(Ljava/lang/String;DDD)V");
        //assertEqualsFloat   = env->GetStaticMethodID(clazz, "assertEquals", "(Ljava/lang/String;FFFV");
        //assertEqualsBoolean = env->GetStaticMethodID(clazz, "assertEquals", "(Ljava/lang/String;ZZ)V");
        //assertEqualsString = env->GetStaticMethodID(clazz, "assertEquals", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
        assertTrue = env->GetStaticMethodID(clazz, "assertTrue", "(Ljava/lang/String;Z)V");
        assertFalse = env->GetStaticMethodID(clazz, "assertFalse", "(Ljava/lang/String;Z)V");
    }

    void checkException(JNIEnv *env, const char* prefix, const char* message) {
        if (env->ExceptionCheck()) {
            ALOGE("SdkNativeTest: %s:%s", prefix, message);
            THROW(JAVA_ASSERT_EXCEPTION);
        }
    }


    template<typename T>
    void assertEquals(JNIEnv *env, jmethodID methodId, const char* message, T const& excepted, T const& actual) {
        LocalEnv<3> lenv(env);
        lenv->CallStaticVoidMethod(clazz, methodId,
                    toJvalue(lenv, message),
                    toJvalue(lenv, excepted),
                    toJvalue(lenv, actual));
        checkException(env, "assertEquals", message);
    }

    template<typename T>
    void assertEquals(JNIEnv* env, jmethodID methodId, const char* message, T const& excepted, T const& actual, T const& delta) {
        LocalEnv<4> lenv(env);
        lenv->CallStaticVoidMethod(clazz, methodId,
                    toJvalue(lenv, message),
                    toJvalue(lenv, excepted),
                    toJvalue(lenv, actual),
                    toJvalue(lenv, delta));

        checkException(env, "assertEquals", message);
    }

    void assertBoolean(JNIEnv* env, jmethodID methodId, const char* message, bool b) {
        LocalEnv<2> lenv(env);
        lenv->CallStaticVoidMethod(clazz, methodId,
                    toJvalue(lenv, message),
                    toJvalue(lenv, b));
        checkException(env, "assertBoolean", message);
    }

};

static JavaAssert asserts;


void JavaAssertInit(JNIEnv* env) {
    asserts.init(env);
}


void JavaAssertEquals(JNIEnv* env, const char* message, long excepted, long actual) {
    asserts.assertEquals(env, asserts.assertEqualsLong, message, excepted, actual);
}

/*void JavaAssertEquals(JNIEnv* env, const char* message, const char* excepted, const char* actual) {
    asserts.assertEquals(env, asserts.assertEqualsString, message, excepted, actual);
}

void JavaAssertEquals(JNIEnv* env, const char* message, bool excepted, bool actual) {
    asserts.assertEquals(env, asserts.assertEqualsBoolean, message, excepted, actual);
}

void JavaAssertEquals(JNIEnv* env, const char* message, int excepted, int actual) {
    asserts.assertEquals(env, asserts.assertEqualsInt, message, excepted, actual);
}
void JavaAssertEquals(JNIEnv* env, const char* message, short excepted, short actual) {
    asserts.assertEquals(env, asserts.assertEqualsShort, message, excepted, actual);
}*/
void JavaAssertEquals(JNIEnv* env, const char* message, double excepted, double actual, double delta) {
    asserts.assertEquals(env, asserts.assertEqualsDouble, message, excepted, actual, delta);
}
/*
void JavaAssertEquals(JNIEnv* env, const char* message, float excepted, float actual, float delta) {
    asserts.assertEquals(env, asserts.assertEqualsFloat, message, excepted, actual, delta);
}*/
void JavaAssertTrue(JNIEnv *env, const char* message, bool actual) {
    asserts.assertBoolean(env, asserts.assertTrue, message, actual);
}

void JavaAssertFalse(JNIEnv *env, const char* message, bool actual) {
    asserts.assertBoolean(env, asserts.assertFalse, message, actual);
}


/////////////////////////////////////
static pthread_key_t try_catch_key;
static pthread_once_t initdone = PTHREAD_ONCE_INIT;

static void init_try_catch_key() {
    pthread_key_create(&try_catch_key, NULL);
}

void  try_block_push(try_block_t* block) {
    pthread_once(&initdone, init_try_catch_key);

    try_block_t* head = (try_block_t*)pthread_getspecific(try_catch_key);
    block->next = head;

    pthread_setspecific(try_catch_key, block);
}

try_block_t* try_block_pop() {
    try_block_t* head = (try_block_t*)pthread_getspecific(try_catch_key);
    if (head != NULL) {
        try_block_t* block = head;
        head = head->next;
        pthread_setspecific(try_catch_key, head);
        return block;
    }
    exit(-1);
    return NULL;
}



