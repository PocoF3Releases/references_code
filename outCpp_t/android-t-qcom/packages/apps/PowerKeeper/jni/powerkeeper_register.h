#ifndef __POWERKEEPER_REGISTER
#define __POWERKEEPER_REGISTER
#include <nativehelper/JNIHelp.h>
#include "jni.h"
#ifndef NELEM
#define NELEM(x0 ((int)(sizeof(x)/sizeof((x)[0])))
#endif
#define REG_JNI(name)    { name }

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof((x))/sizeof(((x)[0])))
#endif

struct RegJNIRec {
    int (*mProc)(JNIEnv*);
};

int register_com_miui_powerkeeper_EventLogManager(JNIEnv* env);
int register_com_miui_powerkeeper_Millet(JNIEnv *env);

static const RegJNIRec gRegJNI[] = {
    REG_JNI(register_com_miui_powerkeeper_EventLogManager),
    REG_JNI(register_com_miui_powerkeeper_Millet)
};

#endif
