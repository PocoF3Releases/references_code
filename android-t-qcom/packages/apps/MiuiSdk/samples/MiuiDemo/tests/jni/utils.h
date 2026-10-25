#ifndef SDKNATIVE_TEST_H
#define SDKNATIVE_TEST_H

#include <jni.h>
#include <setjmp.h>

#define JAVA_ASSERT_EXCEPTION 1

void JavaAssertInit(JNIEnv* env);
void JavaAssertEquals(JNIEnv* env, const char* message, long excepted, long actual);
//void JavaAssertEquals(JNIEnv* env, const char* message, const char* expected, const char* actual);
//void JavaAssertEquals(JNIEnv* env, const char* message, bool excepted, bool actual);
//void JavaAssertEquals(JNIEnv* env, const char* message, int excepted, int actual);
//void JavaAssertEquals(JNIEnv* env, const char* message, short excepted, short actual);
void JavaAssertEquals(JNIEnv* env, const char* message, double expected, double actual, double delta);
//void JavaAssertEquals(JNIEnv* env, const char* message, float excepted, float actual, float delta);
void JavaAssertTrue(JNIEnv *env, const char* message, bool b);
void JavaAssertFalse(JNIEnv *env, const char* message, bool b);

//////////////////////////////////////////
//try-catch
typedef struct _try_block_t try_block_t;
struct _try_block_t {
    jmp_buf buf;
    try_block_t *next;
};

void try_block_push(try_block_t* block);
try_block_t* try_block_pop();

#define TRY do { try_block_t _try_block_; \
        try_block_push(&_try_block_); \
        switch(setjmp(_try_block_.buf)) { case 0:

#define CATCH(x)  break; case x:
#define DEFAULT_CATCH  break; default:
#define END_TRY  }}while(0);
#define THROW(x) longjmp(try_block_pop()->buf, x)



#endif
