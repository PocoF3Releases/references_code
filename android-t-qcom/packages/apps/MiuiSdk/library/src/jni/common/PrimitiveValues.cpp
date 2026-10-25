/*
 * Copyright (C) 2013, Xiaomi Inc. All rights reserved.
 */

#define LOG_TAG "com_miui_Common-JNI"

#include <stdio.h>
#include <utils/Log.h>
#include <jni.h>

#include "PrimitiveValues.h"

namespace miui {

jclass PrimitiveValues::sBoolean = NULL;
jclass PrimitiveValues::sByte = NULL;
jclass PrimitiveValues::sCharacter = NULL;
jclass PrimitiveValues::sShort = NULL;
jclass PrimitiveValues::sInteger = NULL;
jclass PrimitiveValues::sLong = NULL;
jclass PrimitiveValues::sFloat = NULL;
jclass PrimitiveValues::sDouble = NULL;

jmethodID PrimitiveValues::sBoolean_valueOf = NULL;
jmethodID PrimitiveValues::sByte_valueOf = NULL;
jmethodID PrimitiveValues::sCharacter_valueOf = NULL;
jmethodID PrimitiveValues::sShort_valueOf = NULL;
jmethodID PrimitiveValues::sInteger_valueOf = NULL;
jmethodID PrimitiveValues::sLong_valueOf = NULL;
jmethodID PrimitiveValues::sFloat_valueOf = NULL;
jmethodID PrimitiveValues::sDouble_valueOf = NULL;

jmethodID PrimitiveValues::sBoolean_booleanValue = NULL;
jmethodID PrimitiveValues::sByte_byteValue = NULL;
jmethodID PrimitiveValues::sCharacter_charValue = NULL;
jmethodID PrimitiveValues::sShort_shortValue = NULL;
jmethodID PrimitiveValues::sInteger_intValue = NULL;
jmethodID PrimitiveValues::sLong_longValue = NULL;
jmethodID PrimitiveValues::sFloat_floatValue = NULL;
jmethodID PrimitiveValues::sDouble_doubleValue = NULL;

bool PrimitiveValues::Init(JNIEnv* env) {
    sBoolean = (jclass) env->NewGlobalRef(env->FindClass("java/lang/Boolean"));
    sBoolean_valueOf = env->GetStaticMethodID(sBoolean, "valueOf", "(Z)Ljava/lang/Boolean;");
    sBoolean_booleanValue = env->GetMethodID(sBoolean, "booleanValue", "()Z");
    if (sBoolean_valueOf == NULL || sBoolean_booleanValue == NULL) {
        ALOGE("Cannot find class java/lang/Boolean");
        return false;
    }

    sByte = (jclass) env->NewGlobalRef(env->FindClass("java/lang/Byte"));
    sByte_valueOf = env->GetStaticMethodID(sByte, "valueOf", "(B)Ljava/lang/Byte;");
    sByte_byteValue = env->GetMethodID(sByte, "byteValue", "()B");
    if (sByte_valueOf == NULL || sByte_byteValue == NULL) {
        ALOGE("Cannot find class java/lang/Byte");
        return false;
    }

    sCharacter = (jclass) env->NewGlobalRef(env->FindClass("java/lang/Character"));
    sCharacter_valueOf = env->GetStaticMethodID(sCharacter, "valueOf", "(C)Ljava/lang/Character;");
    sCharacter_charValue = env->GetMethodID(sCharacter, "charValue", "()C");
    if (sCharacter_valueOf == NULL || sCharacter_charValue == NULL) {
        ALOGE("Cannot find class java/lang/Character");
        return false;
    }

    sShort = (jclass) env->NewGlobalRef(env->FindClass("java/lang/Short"));
    sShort_valueOf = env->GetStaticMethodID(sShort, "valueOf", "(S)Ljava/lang/Short;");
    sShort_shortValue = env->GetMethodID(sShort, "shortValue", "()S");
    if (sShort_valueOf == NULL || sShort_shortValue == NULL) {
        ALOGE("Cannot find class java/lang/Short");
        return false;
    }

    sInteger = (jclass) env->NewGlobalRef(env->FindClass("java/lang/Integer"));
    sInteger_valueOf = env->GetStaticMethodID(sInteger, "valueOf", "(I)Ljava/lang/Integer;");
    sInteger_intValue = env->GetMethodID(sInteger, "intValue", "()I");
    if (sInteger_valueOf == NULL || sInteger_intValue == NULL) {
        ALOGE("Cannot find class java/lang/Integer");
        return false;
    }

    sLong = (jclass) env->NewGlobalRef(env->FindClass("java/lang/Long"));
    sLong_valueOf = env->GetStaticMethodID(sLong, "valueOf", "(J)Ljava/lang/Long;");
    sLong_longValue = env->GetMethodID(sLong, "longValue", "()J");
    if (sLong_valueOf == NULL || sLong_longValue == NULL) {
        ALOGE("Cannot find class java/lang/Long");
        return false;
    }

    sFloat = (jclass) env->NewGlobalRef(env->FindClass("java/lang/Float"));
    sFloat_valueOf = env->GetStaticMethodID(sFloat, "valueOf", "(F)Ljava/lang/Float;");
    sFloat_floatValue = env->GetMethodID(sFloat, "floatValue", "()F");
    if (sFloat_valueOf == NULL || sFloat_floatValue == NULL) {
        ALOGE("Cannot find class java/lang/Float");
        return false;
    }

    sDouble = (jclass) env->NewGlobalRef(env->FindClass("java/lang/Double"));
    sDouble_valueOf = env->GetStaticMethodID(sDouble, "valueOf", "(D)Ljava/lang/Double;");
    sDouble_doubleValue = env->GetMethodID(sDouble, "doubleValue", "()D");
    if (sDouble_valueOf == NULL || sDouble_doubleValue == NULL) {
        ALOGE("Cannot find class java/lang/Double");
        return false;
    }

    return true;
}

jobject PrimitiveValues::Box(JNIEnv* env, char type, jvalue jv) {
    switch (type) {
        case 'Z':
            return env->CallStaticObjectMethod(sBoolean, sBoolean_valueOf, jv.z);
        case 'B':
            return env->CallStaticObjectMethod(sByte, sByte_valueOf, jv.b);
        case 'C':
            return env->CallStaticObjectMethod(sCharacter, sCharacter_valueOf, jv.c);
        case 'S':
            return env->CallStaticObjectMethod(sShort, sShort_valueOf, jv.s);
        case 'I':
            return env->CallStaticObjectMethod(sInteger, sInteger_valueOf, jv.i);
        case 'J':
            return env->CallStaticObjectMethod(sLong, sLong_valueOf, jv.j);
        case 'F':
            return env->CallStaticObjectMethod(sFloat, sFloat_valueOf, jv.f);
        case 'D':
            return env->CallStaticObjectMethod(sDouble, sDouble_valueOf, jv.d);
        case 'L':
        case '[':
            return jv.l;
    }
    return NULL;
}

void PrimitiveValues::Unbox(JNIEnv* env, char type, jobject obj, jvalue* jv) {
    if (obj == NULL) {
        jv->j = 0;
        return;
    }
    switch (type) {
        case 'Z':
            jv->z = env->CallBooleanMethod(obj, sBoolean_booleanValue);
            return;
        case 'B':
            jv->b = env->CallByteMethod(obj, sByte_byteValue);
            return;
        case 'C':
            jv->c = env->CallCharMethod(obj, sCharacter_charValue);
            return;
        case 'S':
            jv->s = env->CallShortMethod(obj, sShort_shortValue);
            return;
        case 'I':
            jv->i = env->CallIntMethod(obj, sInteger_intValue);
            return;
        case 'J':
            jv->j = env->CallLongMethod(obj, sLong_longValue);
            return;
        case 'F':
            jv->f = env->CallFloatMethod(obj, sFloat_floatValue);
            return;
        case 'D':
            jv->d = env->CallDoubleMethod(obj, sDouble_doubleValue);
            return;
        case '[':
        case 'L':
            jv->l = obj;
    }
}

char PrimitiveValues::Unbox(JNIEnv* env, jobject obj, jvalue* jv) {
    if (obj == NULL) {
        jv->j = 0;
        return '\0';
    }

    jclass clazz = env->GetObjectClass(obj);
    if (env->IsSameObject(clazz, sBoolean)) {
        jv->z = env->CallBooleanMethod(obj, sBoolean_booleanValue);
        return 'Z';
    } else if (env->IsSameObject(clazz, sByte)) {
        jv->b = env->CallByteMethod(obj, sByte_byteValue);
        return 'B';
    } else if (env->IsSameObject(clazz, sCharacter)) {
        jv->c = env->CallCharMethod(obj, sCharacter_charValue);
        return 'C';
    } else if (env->IsSameObject(clazz, sShort)) {
        jv->s = env->CallShortMethod(obj, sShort_shortValue);
        return 'S';
    } else if (env->IsSameObject(clazz, sInteger)) {
        jv->i = env->CallIntMethod(obj, sInteger_intValue);
        return 'I';
    } else if (env->IsSameObject(clazz, sLong)) {
        jv->j = env->CallLongMethod(obj, sLong_longValue);
        return 'J';
    } else if (env->IsSameObject(clazz, sFloat)) {
        jv->f = env->CallFloatMethod(obj, sFloat_floatValue);
        return 'F';
    } else if (env->IsSameObject(clazz, sDouble)) {
        jv->d = env->CallDoubleMethod(obj, sDouble_doubleValue);
        return 'D';
    } else {
        jv->l = obj;
        return 'L';
    }
}

}
