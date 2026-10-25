/*
 * Copyright (C) 2013, Xiaomi Inc. All rights reserved.
 */

#ifndef _MIUI_PRIMITIVE_VALUES_H_
#define _MIUI_PRIMITIVE_VALUES_H_

namespace miui {

struct PrimitiveValues {
    static jclass sBoolean;
    static jclass sByte;
    static jclass sCharacter;
    static jclass sShort;
    static jclass sInteger;
    static jclass sLong;
    static jclass sFloat;
    static jclass sDouble;

    static jmethodID sBoolean_valueOf;
    static jmethodID sByte_valueOf;
    static jmethodID sCharacter_valueOf;
    static jmethodID sShort_valueOf;
    static jmethodID sInteger_valueOf;
    static jmethodID sLong_valueOf;
    static jmethodID sFloat_valueOf;
    static jmethodID sDouble_valueOf;

    static jmethodID sBoolean_booleanValue;
    static jmethodID sByte_byteValue;
    static jmethodID sCharacter_charValue;
    static jmethodID sShort_shortValue;
    static jmethodID sInteger_intValue;
    static jmethodID sLong_longValue;
    static jmethodID sFloat_floatValue;
    static jmethodID sDouble_doubleValue;

    static bool Init(JNIEnv* env);

    static jobject Box(JNIEnv* env, char type, jvalue jv);

    static void Unbox(JNIEnv* env, char type, jobject obj, jvalue* jv);

    static char Unbox(JNIEnv* env, jobject obj, jvalue* jv);
};

}

#endif//_MIUI_PRIMITIVE_VALUES_H_
