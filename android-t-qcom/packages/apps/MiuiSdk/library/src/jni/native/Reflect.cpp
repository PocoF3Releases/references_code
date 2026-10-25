/*
 * Copyright (C) 2013, Xiaomi Inc. All rights reserved.
 */

#include <map>
#include <string>

#include <utils/Log.h>
#include <stdio.h>
#include <stdlib.h>

#include <jni.h>
#include <pthread.h>

#include "Native.h"

#include "Utilities.h"
#include "PrimitiveValues.h"

namespace miui {

enum StaticType {
    STATIC_UNDEFINED_TYPE,
    STATIC_TYPE,
    STATIC_NONE_TYPE,
};

struct MiuiField {
    std::string name;
    bool isStatic;
    char type;
    jclass clazz;
    jfieldID fieldID;
};

struct MiuiMethod {
    MiuiMethod()
    :isStatic(false)
    ,isConstructor(false)
    ,shorty(NULL)
    ,clazz(NULL)
    ,methodID(NULL)
#ifdef ART_UNSUPPORT_NONVIRTUAL
    ,methodTable(NULL)
#endif
    {}
    ~MiuiMethod() {
        if (shorty) {
            delete[] shorty;
        }
#ifdef ART_UNSUPPORT_NONVIRTUAL
        if (methodTable) {
            delete methodTable;
        }
#endif
    }

    std::string name;
    bool isStatic;
    bool isConstructor;
    char *shorty;
    jclass clazz;
    jmethodID methodID;

    inline jmethodID getMethodID(JNIEnv *env, jclass clazz) {
#ifdef ART_UNSUPPORT_NONVIRTUAL
        if (getRuntimeType() == JVM_DALVIK || isStatic || isConstructor
            || env->IsSameObject(clazz,this->clazz) || clazz == NULL) {
            return methodID;
        }
        return getMethodIDByClass(env, clazz);
#else
        return methodID;
#endif
    }

#ifdef ART_UNSUPPORT_NONVIRTUAL
private:
    typedef std::map<int, jmethodID> MethodTable;
    MethodTable *methodTable;
    jmethodID getMethodIDByClass(JNIEnv *env, jclass clazz) {
        jmethodID methodId = 0;
        int hashCode = jobject2HashCode(env, clazz);
        if (methodTable == NULL) {
            methodTable = new MethodTable();
        } else {
            typename MethodTable::iterator it = methodTable->find(hashCode);
            if (it != methodTable->end()) {
                methodId = it->second;
            }
        }
        //reget it
        if (methodId == 0) {
            static const char default_signature[] = "()V";
            //try find it
            char szName[256];
            const char* strname = strrchr(name.c_str(), '.');
            if (strname == NULL) {
                return 0;
            }
            strname ++;
            const char* strsig = strchr(strname, '(');
            if (strsig == NULL) {
                strcpy(szName, strname);
                strsig = default_signature;
            } else {
                int len = strsig - strname;
                strncpy(szName, strname, len);
                szName[len] = 0;
            }
            methodId = env->GetMethodID(clazz, szName, strsig);
            if (methodId != 0) {
                (*methodTable)[hashCode] = methodId;
            }
        }

        return methodId;
    }
#endif
};

jmethodID gGetClassName = NULL;
jclass gMethodClass = NULL;
jclass gConstructorClass = NULL;
jclass gFieldClass = NULL;
jfieldID gMethodPtrField = NULL;
jfieldID gConstructorPtrField = NULL;
jfieldID gFieldPtrField = NULL;


template<class T>
class InstancePtr {
    T * ptr_;
public:
    InstancePtr() : ptr_(NULL) { }
    inline T* get() { return ptr_ ? ptr_ : (ptr_ = new T); }
    inline T* operator->() { return get(); }
    inline T& operator*() { return *get(); }
    ~InstancePtr() { if (ptr_) delete ptr_; }
};

static pthread_mutex_t sNativeMutex;
InstancePtr<std::map<std::string, jclass> > gClasses;
InstancePtr<std::map<std::string, MiuiField*> > gFields;
InstancePtr<std::map<std::string, MiuiMethod*> > gMethods;

static void initMiuiReflectClass(JNIEnv *env, const char *className, jclass *pClass, jfieldID *pFieldID) {
    if (*pFieldID == NULL) {
        if (*pClass == NULL) {
            *pClass = env->FindClass(className);
            if (*pClass != NULL) {
                *pClass = (jclass) env->NewGlobalRef(*pClass);
            }
        }
        if (*pClass != NULL) {
            *pFieldID = env->GetFieldID(*pClass, "mPtr", "J");
            if (*pFieldID == NULL) {
                throwNew(env, "miui/reflect/NoSuchFieldException", "Couldn't find class %s.mPtr", className);
            }
        } else {
            throwNew(env, "miui/reflect/NoSuchClassException", "Couldn't find class %s", className);
        }
    }
}

static jobject createMethod(JNIEnv *env, MiuiMethod* method) {
    initMiuiReflectClass(env, "miui/reflect/Method", &gMethodClass, &gMethodPtrField);

    if (gMethodPtrField != NULL) {
        jobject obj = env->AllocObject(gMethodClass);
        if (env->ExceptionCheck()) {
            return NULL;
        }
        env->SetLongField(obj, gMethodPtrField, reinterpret_cast<jlong>(method));
        return obj;
    }
    return NULL;
}

static MiuiMethod* getMethod(JNIEnv *env, jobject object) {
    initMiuiReflectClass(env, "miui/reflect/Method", &gMethodClass, &gMethodPtrField);

    if (gMethodPtrField != NULL) {
        MiuiMethod* m = reinterpret_cast<MiuiMethod*>(env->GetLongField(object, gMethodPtrField));
        if (m == NULL) {
            throwNew(env, "miui/reflect/NullPointerException", "mPtr is null");
        }
        return m;
    }
    return NULL;
}

static jobject createConstructor(JNIEnv *env, MiuiMethod* method) {
    initMiuiReflectClass(env, "miui/reflect/Constructor", &gConstructorClass, &gConstructorPtrField);

    if (gConstructorPtrField != NULL) {
        jobject obj = env->AllocObject(gConstructorClass);
        if (env->ExceptionCheck()) {
            return NULL;
        }
        env->SetLongField(obj, gConstructorPtrField, reinterpret_cast<jlong>(method));
        return obj;
    }
    return NULL;
}

static MiuiMethod* getConstructor(JNIEnv *env, jobject object) {
    initMiuiReflectClass(env, "miui/reflect/Constructor", &gConstructorClass, &gConstructorPtrField);

    if (gConstructorPtrField != NULL) {
        MiuiMethod* m = reinterpret_cast<MiuiMethod*>(env->GetLongField(object, gConstructorPtrField));
        if (m == NULL) {
            throwNew(env, "miui/reflect/NullPointerException", "mPtr is null");
        }
        return m;
    }
    return NULL;
}


static jobject createField(JNIEnv *env, MiuiField* field) {
    initMiuiReflectClass(env, "miui/reflect/Field", &gFieldClass, &gFieldPtrField);

    if (gFieldPtrField != NULL) {
        jobject obj = env->AllocObject(gFieldClass);
        if (env->ExceptionCheck()) {
            return NULL;
        }
        env->SetLongField(obj, gFieldPtrField, reinterpret_cast<jlong>(field));
        return obj;
    }
    return NULL;
}

static MiuiField* getField(JNIEnv *env, jobject object) {
    initMiuiReflectClass(env, "miui/reflect/Field", &gFieldClass, &gFieldPtrField);

    if (gFieldPtrField != NULL) {
        MiuiField* f = reinterpret_cast<MiuiField*>(env->GetLongField(object, gFieldPtrField));
        if (f == NULL) {
            throwNew(env, "miui/reflect/NullPointerException", "mPtr is null");
        }
        return f;
    }
    return NULL;
}

static jstring getClassName(JNIEnv *env, jclass clazz) {
    if (gGetClassName == NULL){
        gGetClassName = env->GetMethodID(env->GetObjectClass(clazz), "getName", "()Ljava/lang/String;");
        if (gGetClassName == NULL){
            throwNew(env, "miui/reflect/NoSuchMethodException", "Couldn't find getName()Ljava/lang/String; method in class Class<?>");
            return NULL;
        }
    }

    return (jstring) env->CallObjectMethod(clazz, gGetClassName);
}

static std::string& toString(JNIEnv *env, jstring str, std::string& result){
    if (str == 0) {
        result = "";
        return result;
    }
    int length = env->GetStringUTFLength(str);
    const char* chars = env->GetStringUTFChars(str, NULL);
    result.assign(chars, length);
    env->ReleaseStringUTFChars(str, chars);
    return result;
}

static std::string& getJniClassName(JNIEnv *env, jstring str, std::string& result) {
    toString(env, str, result);
    std::replace(&(result[0]), (&(result[result.length()]) + 1), '.', '/');
    return result;
}

static std::string getJniClassName(JNIEnv *env, jstring str) {
    std::string result;
    return getJniClassName(env, str, result);
}

static std::string getMapIndex(std::string& strClassName, std::string& strFieldName) {
    return strClassName + "." + strFieldName;
}

static std::string getMapIndex(std::string& strClassName, std::string& methodName, std::string& signature) {
    return strClassName + "." + methodName + signature;
}

static jclass getClass(JNIEnv *env, jstring className) {
    std::string strClassName;
    getJniClassName(env, className, strClassName);
    pthread_mutex_lock(&sNativeMutex);
    jclass& clazz = (*gClasses)[strClassName];
    if (clazz == NULL) {
        clazz = env->FindClass(strClassName.c_str());
        if(clazz == NULL || env->ExceptionCheck()) {
            env->ExceptionClear();
        } else {
            clazz = (jclass) env->NewGlobalRef(clazz);
        }
    }
    pthread_mutex_unlock(&sNativeMutex);

    if (clazz == NULL) {
        throwNew(env, "miui/reflect/NoSuchClassException", "Couldn't find class %s", strClassName.c_str());
    }
    return clazz;
}

static MiuiField* getField(JNIEnv *env, jclass clazz, jstring className, jstring fieldName, jstring type, int staticType = STATIC_UNDEFINED_TYPE, jobject reflect = NULL ) {
    std::string strClassName, strFieldName, strFieldType;
    getJniClassName(env, className, strClassName);
    toString(env, fieldName, strFieldName);
    toString(env, type, strFieldType);
    std::string strMapIndex = getMapIndex(strClassName, strFieldName);

    pthread_mutex_lock(&sNativeMutex);
    MiuiField*& field = (*gFields)[strMapIndex];
    if (field == NULL) {
        jfieldID fieldID = NULL;
        bool isStatic = staticType == STATIC_TYPE;

        if (reflect != NULL) {
            fieldID = env->FromReflectedField(reflect);
        } else {
            if (staticType != STATIC_TYPE) {
                fieldID = env->GetFieldID(clazz, strFieldName.c_str(), strFieldType.c_str());
            }

            if (fieldID == NULL && staticType != STATIC_NONE_TYPE) {
                env->ExceptionClear();
                fieldID = env->GetStaticFieldID(clazz, strFieldName.c_str(), strFieldType.c_str());
                isStatic = true;
            }
        }

        if (fieldID == NULL) {
            env->ExceptionClear();
        } else {
            field = new MiuiField();
            field->isStatic = isStatic;
            field->type = strFieldType[0];
            field->clazz = (jclass) env->NewGlobalRef(clazz);
            field->fieldID = fieldID;
            field->name = strMapIndex;
        }
    }
    pthread_mutex_unlock(&sNativeMutex);

    if (field == NULL) {
        throwNew(env, "miui/reflect/NoSuchFieldException", "Couldn't find field %s", strMapIndex.c_str());
    }
    return field;
}

static MiuiMethod* getMethod(JNIEnv *env, jclass clazz, jstring className, jstring methodName, jstring signature, int staticType = STATIC_UNDEFINED_TYPE, jobject reflect = 0) {
    std::string strClassName, strMethodName, strSignature;
    getJniClassName(env, className, strClassName);
    if (methodName == NULL) {
        strMethodName = "<init>";
    } else {
        toString(env, methodName, strMethodName);
    }
    toString(env, signature, strSignature);
    std::string strMapIndex = getMapIndex(strClassName, strMethodName, strSignature);
    pthread_mutex_lock(&sNativeMutex);
    MiuiMethod*& method = (*gMethods)[strMapIndex];
    if (method == NULL) {
        bool isConstructor = methodName == NULL;
        jmethodID methodID = NULL;
        bool isStatic = staticType == STATIC_TYPE;
        if (reflect != NULL) {
            methodID = env->FromReflectedMethod(reflect);
        } else {
            if (staticType != STATIC_TYPE) {
                methodID = env->GetMethodID(clazz, strMethodName.c_str(), strSignature.c_str());
            }

            if (methodID == NULL && !isConstructor && staticType != STATIC_NONE_TYPE){
                env->ExceptionClear();
                methodID = env->GetStaticMethodID(clazz, strMethodName.c_str(), strSignature.c_str());
                isStatic = true;
            }
        }
        if (methodID == NULL) {
            env->ExceptionClear();
        } else {
            method = new MiuiMethod();
            method->isConstructor = isConstructor;
            method->isStatic = isStatic;
            method->clazz = (jclass) env->NewGlobalRef(clazz);
            method->methodID = methodID;
            method->shorty = generateShorty(strSignature.c_str(), NULL);
            method->name = strMapIndex;
        }
    }
    pthread_mutex_unlock(&sNativeMutex);

    if (method == NULL) {
        throwNew(env, "miui/reflect/NoSuchMethodException", "Couldn't find method %s", strMapIndex.c_str());
    }
    return method;
}

static jobject com_miui_internal_os_Native_getMethodByClass(JNIEnv *env, jclass, jclass clazz, jstring methodName, jstring signature) {
    MiuiMethod* method = getMethod(env, clazz, getClassName(env, clazz), methodName, signature);
    if (method == NULL) {
        return NULL;
    }

    return createMethod(env, method);
}

static jobject com_miui_internal_os_Native_getMethodByName(JNIEnv *env, jclass, jstring className, jstring methodName, jstring signature) {
    jclass clazz = getClass(env, className);
    if (clazz == NULL) {
        return NULL;
    }

    MiuiMethod* method = getMethod(env, clazz, className, methodName, signature);
    if (method == NULL) {
        return NULL;
    }

    return createMethod(env, method);
}

static jobject com_miui_internal_os_Native_getMethodByReflectMethod(JNIEnv *env, jclass, jclass clazz, jstring className, jstring methodName, jstring signature, jboolean isStatic, jobject reflect) {

    MiuiMethod* method = getMethod(env, clazz, className, methodName, signature, isStatic ? STATIC_TYPE : STATIC_NONE_TYPE, reflect);

    if (method == NULL) {
        return NULL;
    }

    return createMethod(env, method);
}

static jobject com_miui_internal_os_Native_getReflectMethod(JNIEnv *env, jclass, jobject method) {
    MiuiMethod* m = getMethod(env, method);
    if (m == NULL) {
        return NULL;
    }
    return env->ToReflectedMethod(m->clazz, m->methodID, m->isStatic);
}

static jvalue* convertArguments(JNIEnv *env, MiuiMethod* method, jobjectArray arguments) {
    int length = env->GetArrayLength(arguments);
    jvalue* args = new jvalue[length];
    for (const char *s = method->shorty + 1; *s != 0; ++s) {
        int index = s - method->shorty - 1;
        switch (*s) {
            case 'Z':
            case 'B':
            case 'C':
            case 'S':
            case 'I':
            case 'J':
            case 'F':
            case 'D':
                PrimitiveValues::Unbox(env, *s, env->GetObjectArrayElement(arguments, index), &args[index]);
                if (env->ExceptionCheck()) {
                    delete [] args;
                    throwNew(env, "miui/reflect/IllegalArgumentException", "Method %s has invalid %d argument", method->name.c_str(), (index + 1));
                    return NULL;
                }
                break;
            case '[':
            case 'L':
                args[index].l = env->GetObjectArrayElement(arguments, index);
                break;
            default:
                ALOGE("Unknown parameter %c", *s);
                args[index].l = env->GetObjectArrayElement(arguments, index);
        }
    }

    return args;
}

static jvalue* convertConstructorArguments(JNIEnv *env, jobject constructorObject, jobjectArray arguments, MiuiMethod** pMethod) {
    MiuiMethod* method = getConstructor(env, constructorObject);
    if (method == NULL) {
        return NULL;
    }

    if (pMethod != NULL) {
        *pMethod = method;
    }
    return convertArguments(env, method, arguments);
}

static jvalue* convertMethodArguments(JNIEnv *env, jobject methodObject, jobjectArray arguments, MiuiMethod** pMethod) {
    MiuiMethod* method = getMethod(env, methodObject);
    if (method == NULL) {
        return NULL;
    }

    if (pMethod != NULL) {
        *pMethod = method;
    }
    return convertArguments(env, method, arguments);
}

class ArgsDeleter {
public:
    ArgsDeleter(jvalue* args) {
        _args = args;
    }

    ~ArgsDeleter() {
        delete [] _args;
    }

private:
    jvalue* _args;
};

#define COM_MIUI_INTERNAL_OS_NATIVE_INVOKE(_ctype, _jname, _type, _retfail, _retok) \
    static _ctype com_miui_internal_os_Native_invoke##_jname(JNIEnv *env, jclass, jobject methodObject, jclass clazz, jobject object, jobjectArray arguments) { \
        MiuiMethod* method; \
        jvalue* args = convertMethodArguments(env, methodObject, arguments, &method); \
        if (args == NULL) { \
            return _retfail; \
        } \
        \
        ArgsDeleter argsDeleter(args); \
        if ((_type != 'L' && method->shorty[0] != _type) || (_type == 'L' && method->shorty[0] != '[' && method->shorty[0] != 'L')) { \
            throwNew(env, "miui/reflect/IllegalArgumentException", "Method %s doesn't return %c but %c", method->name.c_str(), _type, method->shorty[0]); \
            return _retfail; \
        } \
        \
        if (clazz == NULL) { \
            if (object == NULL) { \
                clazz = method->clazz; \
            } else { \
                clazz = (jclass) env->GetObjectClass(object); \
            } \
        } \
        jmethodID methodID = method->getMethodID(env, clazz); \
        if (methodID == 0) { \
            throwNew(env, "miui/reflect/IllegalArgumentException", "Method %s is not exits in class %s", method->name.c_str(), getJniClassName(env, getClassName(env, clazz)).c_str()); \
            return _retfail; \
        } \
        jvalue result; \
        if (method->isStatic) { \
            if (!env->IsAssignableFrom(clazz, method->clazz)) { \
                throwNew(env, "miui/reflect/IllegalArgumentException", "Method %s is not belong to class %s", method->name.c_str(), getJniClassName(env, getClassName(env, clazz)).c_str()); \
                _retok = _retfail; \
            } else { \
                _retok = env->CallStatic##_jname##MethodA(clazz, methodID, args); \
            } \
        } else { \
            if (object == NULL) { \
                throwNew(env, "java/lang/NullPointerException", "Method %s invoked by null object", method->name.c_str()); \
                _retok = _retfail; \
            } else if (!env->IsInstanceOf(object, clazz)) { \
                throwNew(env, "miui/reflect/IllegalArgumentException", "Method %s is not belong to class %s", method->name.c_str(), getJniClassName(env, getClassName(env, clazz)).c_str()); \
                _retok = _retfail; \
            } else { \
                _retok = env->CallNonvirtual##_jname##MethodA(object, clazz, methodID, args); \
            } \
        } \
        return _retok; \
    }

static jobject com_miui_internal_os_Native_invokeObject(JNIEnv *env, jclass, jobject methodObject, jclass clazz, jobject object, jobjectArray arguments) {
    MiuiMethod* method;
    jvalue* args = convertMethodArguments(env, methodObject, arguments, &method);
    if (args == NULL) {
        return NULL;
    }
    ArgsDeleter argsDeleter(args);
    if (clazz == NULL) {
        if (object == NULL) {
            clazz = method->clazz;
        } else {
            clazz = (jclass) env->GetObjectClass(object);
        }
    }

    jmethodID methodID = method->getMethodID(env, clazz);

    if (methodID == 0) {
        throwNew(env, "miui/reflect/IllegalArgumentException", "Method %s is not exits in class %s", method->name.c_str(), getJniClassName(env, getClassName(env, clazz)).c_str());
        return NULL;
    }

    jvalue result;
    result.l = NULL;
    if (method->isStatic) {
        if (!env->IsAssignableFrom(clazz, method->clazz)) {
            throwNew(env, "miui/reflect/IllegalArgumentException", "Method %s is not belong to class %s", method->name.c_str(), getJniClassName(env, getClassName(env, clazz)).c_str());
        } else {
            switch (method->shorty[0]) {
                case 'V':
                    env->CallStaticVoidMethodA(clazz, methodID, args);
                    break;
                case 'Z':
                    result.z = env->CallStaticBooleanMethodA(clazz, methodID, args);
                    break;
                case 'B':
                    result.b = env->CallStaticByteMethodA(clazz, methodID, args);
                    break;
                case 'C':
                    result.c = env->CallStaticCharMethodA(clazz, methodID, args);
                    break;
                case 'S':
                    result.s = env->CallStaticShortMethodA(clazz, methodID, args);
                    break;
                case 'I':
                    result.i = env->CallStaticIntMethodA(clazz, methodID, args);
                    break;
                case 'J':
                    result.j = env->CallStaticLongMethodA(clazz, methodID, args);
                    break;
                case 'F':
                    result.f = env->CallStaticFloatMethodA(clazz, methodID, args);
                    break;
                case 'D':
                    result.d = env->CallStaticDoubleMethodA(clazz, methodID, args);
                    break;
                case '[':
                case 'L':
                    result.l = env->CallStaticObjectMethodA(clazz, methodID, args);
                    break;
            }
        }
    } else {
        if (object == NULL) {
            throwNew(env, "java/lang/NullPointerException", "Method %s invoked by null object", method->name.c_str());
        } else if (!env->IsInstanceOf(object, clazz)) {
            throwNew(env, "miui/reflect/IllegalArgumentException", "Method %s is not belong to class %s", method->name.c_str(), getJniClassName(env, getClassName(env, clazz)).c_str());
        } else {
            switch (method->shorty[0]) {
                case 'V':
                    env->CallNonvirtualVoidMethodA(object, clazz, methodID, args);
                    break;
                case 'Z':
                    result.z = env->CallNonvirtualBooleanMethodA(object, clazz, methodID, args);
                    break;
                case 'B':
                    result.b = env->CallNonvirtualByteMethodA(object, clazz, methodID, args);
                    break;
                case 'C':
                    result.c = env->CallNonvirtualCharMethodA(object, clazz, methodID, args);
                    break;
                case 'S':
                    result.s = env->CallNonvirtualShortMethodA(object, clazz, methodID, args);
                    break;
                case 'I':
                    result.i = env->CallNonvirtualIntMethodA(object, clazz, methodID, args);
                    break;
                case 'J':
                    result.j = env->CallNonvirtualLongMethodA(object, clazz, methodID, args);
                    break;
                case 'F':
                    result.f = env->CallNonvirtualFloatMethodA(object, clazz, methodID, args);
                    break;
                case 'D':
                    result.d = env->CallNonvirtualDoubleMethodA(object, clazz, methodID, args);
                    break;
                case '[':
                case 'L':
                    result.l = env->CallNonvirtualObjectMethodA(object, clazz, methodID, args);
                    break;
            }
        }
    }

    if (env->ExceptionCheck()) {
        return NULL;
    }

    return PrimitiveValues::Box(env, method->shorty[0], result);
}

static void com_miui_internal_os_Native_invoke(JNIEnv *env, jclass, jobject methodObject, jclass clazz, jobject object, jobjectArray arguments) {
    com_miui_internal_os_Native_invokeObject(env, NULL, methodObject, clazz, object, arguments);
}

COM_MIUI_INTERNAL_OS_NATIVE_INVOKE(jboolean, Boolean, 'Z', false, result.z);
COM_MIUI_INTERNAL_OS_NATIVE_INVOKE(jbyte, Byte, 'B', 0, result.b);
COM_MIUI_INTERNAL_OS_NATIVE_INVOKE(jchar, Char, 'C', 0, result.c);
COM_MIUI_INTERNAL_OS_NATIVE_INVOKE(jshort, Short, 'S', 0, result.s);
COM_MIUI_INTERNAL_OS_NATIVE_INVOKE(jint, Int, 'I', 0, result.i);
COM_MIUI_INTERNAL_OS_NATIVE_INVOKE(jlong, Long, 'J', 0, result.j);
COM_MIUI_INTERNAL_OS_NATIVE_INVOKE(jfloat, Float, 'F', 0.0f, result.f);
COM_MIUI_INTERNAL_OS_NATIVE_INVOKE(jdouble, Double, 'D', 0.0, result.d);

static jobject com_miui_internal_os_Native_getConstructorByClass(JNIEnv *env, jclass, jclass clazz, jstring signature) {
    MiuiMethod* method = getMethod(env, clazz, getClassName(env, clazz), NULL, signature);
    if (method == NULL) {
        return NULL;
    }

    return createConstructor(env, method);
}

static jobject com_miui_internal_os_Native_getConstructorByName(JNIEnv *env, jclass, jstring className, jstring signature) {
    jclass clazz = getClass(env, className);
    if (clazz == NULL) {
        return NULL;
    }

    MiuiMethod* method = getMethod(env, clazz, className, NULL, signature);
    if (method == NULL) {
        return NULL;
    }

    return createConstructor(env, method);
}

static jobject com_miui_internal_os_Native_getConstructorByReflectConstructor(JNIEnv *env, jclass, jclass clazz, jstring className, jstring signature, jobject reflect) {

    MiuiMethod* method = getMethod(env, clazz, className, NULL, signature, STATIC_NONE_TYPE, reflect);
    if (method == NULL) {
        return NULL;
    }

    return createConstructor(env, method);
}

static jobject com_miui_internal_os_Native_getReflectConstructor(JNIEnv *env, jclass, jobject constructor) {
    MiuiMethod* m = getConstructor(env, constructor);
    if (m == NULL) {
        return NULL;
    }
    return env->ToReflectedMethod(m->clazz, m->methodID, m->isStatic);
}

static jobject com_miui_internal_os_Native_newInstance(JNIEnv *env, jclass, jobject constructor, jobjectArray arguments) {
    MiuiMethod* method;
    jvalue* args = convertConstructorArguments(env, constructor, arguments, &method);
    if (args == NULL) {
        return NULL;
    }

    jobject object = env->NewObjectA(method->clazz, method->methodID, args);
    delete [] args;
    return object;
}

static jobject com_miui_internal_os_Native_getFieldByClass(JNIEnv *env, jclass, jclass clazz, jstring fieldName, jstring signature) {
    MiuiField* field = getField(env, clazz, getClassName(env, clazz), fieldName, signature);
    if (field == NULL) {
        return NULL;
    }

    return createField(env, field);
}

static jobject com_miui_internal_os_Native_getFieldByName(JNIEnv *env, jclass, jstring className, jstring fieldName, jstring signature) {
    jclass clazz = getClass(env, className);
    if (clazz == NULL) {
        return NULL;
    }

    MiuiField* field = getField(env, clazz, className, fieldName, signature);
    if (field == NULL) {
        return NULL;
    }

    return createField(env, field);
}

static jobject com_miui_internal_os_Native_getFieldByReflectField(JNIEnv *env, jclass, jclass clazz, jstring className, jstring fieldName, jstring signature, jboolean isStatic, jobject reflect) {

    MiuiField* field = getField(env, clazz, className, fieldName, signature, isStatic ? STATIC_TYPE : STATIC_NONE_TYPE, reflect);

    if (field == NULL) {
        return NULL;
    }

    return createField(env, field);
}

static jobject com_miui_internal_os_Native_getReflectField(JNIEnv *env, jclass, jobject field) {
    MiuiField* f = getField(env, field);
    if (f == NULL) {
        return NULL;
    }
    return env->ToReflectedField(f->clazz, f->fieldID, f->isStatic);
}

#define COM_MIUI_INTERNAL_OS_NATIVE_SETFIELDVALUE(_ctype, _jname, _type) \
    static void com_miui_internal_os_Native_set##_jname##FieldValue(JNIEnv *env, jclass, jobject fieldObject, jobject object, _ctype value) { \
        MiuiField* field = getField(env, fieldObject); \
        if (field == NULL) { \
            return; \
        } \
        \
        if (field->type != _type) { \
            throwNew(env, "miui/reflect/IllegalArgumentException", "Field %s isn't type %c but %c", field->name.c_str(), _type, field->type); \
            return; \
        } \
        \
        if (field->isStatic) { \
            env->SetStatic##_jname##Field(field->clazz, field->fieldID, value); \
        } else { \
            if (object == NULL) { \
                throwNew(env, "java/lang/NullPointerException", "Field %s set to null object", field->name.c_str()); \
            } else if (!env->IsInstanceOf(object, field->clazz)) { \
                throwNew(env, "miui/reflect/IllegalArgumentException", "Field %s is not belong to class %s", field->name.c_str(), getJniClassName(env, getClassName(env, (jclass) env->GetObjectClass(object))).c_str()); \
            } else {\
                env->Set##_jname##Field(object, field->fieldID, value); \
            } \
        } \
    }

COM_MIUI_INTERNAL_OS_NATIVE_SETFIELDVALUE(jboolean, Boolean, 'Z');
COM_MIUI_INTERNAL_OS_NATIVE_SETFIELDVALUE(jbyte, Byte, 'B');
COM_MIUI_INTERNAL_OS_NATIVE_SETFIELDVALUE(jchar, Char, 'C');
COM_MIUI_INTERNAL_OS_NATIVE_SETFIELDVALUE(jshort, Short, 'S');
COM_MIUI_INTERNAL_OS_NATIVE_SETFIELDVALUE(jint, Int, 'I');
COM_MIUI_INTERNAL_OS_NATIVE_SETFIELDVALUE(jlong, Long, 'J');
COM_MIUI_INTERNAL_OS_NATIVE_SETFIELDVALUE(jfloat, Float, 'F');
COM_MIUI_INTERNAL_OS_NATIVE_SETFIELDVALUE(jdouble, Double, 'D');
COM_MIUI_INTERNAL_OS_NATIVE_SETFIELDVALUE(jobject, Object, 'L');

#define COM_MIUI_INTERNAL_OS_NATIVE_GETFIELDVALUE(_ctype, _jname, _type, _retfail, _retok) \
    static _ctype com_miui_internal_os_Native_get##_jname##FieldValue(JNIEnv *env, jclass, jobject fieldObject, jobject object) { \
        MiuiField* field = getField(env, fieldObject); \
        if (field == NULL) { \
            return _retfail; \
        } \
        \
        if (field->type != _type) { \
            throwNew(env, "miui/reflect/IllegalArgumentException", "Field %s isn't type %c but %c", field->name.c_str(), _type, field->type); \
            return _retfail; \
        } \
        \
        jvalue result; \
        if (field->isStatic) { \
            _retok = env->GetStatic##_jname##Field(field->clazz, field->fieldID); \
        } else { \
            if (object == NULL) { \
                throwNew(env, "java/lang/NullPointerException", "Field %s get from null object", field->name.c_str()); \
                _retok = _retfail; \
            } else if (!env->IsInstanceOf(object, field->clazz)) { \
                throwNew(env, "miui/reflect/IllegalArgumentException", "Field %s is not belong to class %s", field->name.c_str(), getJniClassName(env, getClassName(env, (jclass) env->GetObjectClass(object))).c_str()); \
                _retok = _retfail; \
            } else { \
                _retok = env->Get##_jname##Field(object, field->fieldID); \
            } \
        } \
        return _retok; \
    }

static jobject com_miui_internal_os_Native_getObjectFieldValue(JNIEnv *env, jclass, jobject fieldObject, jobject object) {
    MiuiField* field = getField(env, fieldObject);
    if (field == NULL) {
        return NULL;
    }

    jvalue result;
    result.l = NULL;
    if (field->isStatic) {
        switch (field->type) {
            case 'Z':
                result.z = env->GetStaticBooleanField(field->clazz, field->fieldID);
                break;
            case 'B':
                result.b = env->GetStaticByteField(field->clazz, field->fieldID);
                break;
            case 'C':
                result.c = env->GetStaticCharField(field->clazz, field->fieldID);
                break;
            case 'S':
                result.s = env->GetStaticShortField(field->clazz, field->fieldID);
                break;
            case 'I':
                result.i = env->GetStaticIntField(field->clazz, field->fieldID);
                break;
            case 'J':
                result.j = env->GetStaticLongField(field->clazz, field->fieldID);
                break;
            case 'F':
                result.f = env->GetStaticFloatField(field->clazz, field->fieldID);
                break;
            case 'D':
                result.d = env->GetStaticDoubleField(field->clazz, field->fieldID);
                break;
            case '[':
            case 'L':
                result.l = env->GetStaticObjectField(field->clazz, field->fieldID);
                break;
        }
    } else {
        if (object == NULL) {
            throwNew(env, "java/lang/NullPointerException", "Field %s get from null object", field->name.c_str());
            result.l = NULL;
        } else if (!env->IsInstanceOf(object, field->clazz)) {
            throwNew(env, "miui/reflect/IllegalArgumentException", "Field %s is not belong to class %s", field->name.c_str(), getJniClassName(env, getClassName(env, (jclass) env->GetObjectClass(object))).c_str());
            result.l = NULL;
        } else {
            switch (field->type) {
                case 'Z':
                    result.z = env->GetBooleanField(object, field->fieldID);
                    break;
                case 'B':
                    result.b = env->GetByteField(object, field->fieldID);
                    break;
                case 'C':
                    result.c = env->GetCharField(object, field->fieldID);
                    break;
                case 'S':
                    result.s = env->GetShortField(object, field->fieldID);
                    break;
                case 'I':
                    result.i = env->GetIntField(object, field->fieldID);
                    break;
                case 'J':
                    result.j = env->GetLongField(object, field->fieldID);
                    break;
                case 'F':
                    result.f = env->GetFloatField(object, field->fieldID);
                    break;
                case 'D':
                    result.d = env->GetDoubleField(object, field->fieldID);
                    break;
                case '[':
                case 'L':
                    result.l = env->GetObjectField(object, field->fieldID);
                    break;
            }
        }
    }

    if (env->ExceptionCheck()) {
        return NULL;
    }

    return PrimitiveValues::Box(env, field->type, result);
}

COM_MIUI_INTERNAL_OS_NATIVE_GETFIELDVALUE(jboolean, Boolean, 'Z', false, result.z);
COM_MIUI_INTERNAL_OS_NATIVE_GETFIELDVALUE(jbyte, Byte, 'B', 0, result.b);
COM_MIUI_INTERNAL_OS_NATIVE_GETFIELDVALUE(jchar, Char, 'C', 0, result.c);
COM_MIUI_INTERNAL_OS_NATIVE_GETFIELDVALUE(jshort, Short, 'S', 0, result.s);
COM_MIUI_INTERNAL_OS_NATIVE_GETFIELDVALUE(jint, Int, 'I', 0, result.i);
COM_MIUI_INTERNAL_OS_NATIVE_GETFIELDVALUE(jlong, Long, 'J', 0, result.j);
COM_MIUI_INTERNAL_OS_NATIVE_GETFIELDVALUE(jfloat, Float, 'F', 0.0f, result.f);
COM_MIUI_INTERNAL_OS_NATIVE_GETFIELDVALUE(jdouble, Double, 'D', 0.0, result.d);

static JNINativeMethod methods[] = {
    {"getMethod", "(Ljava/lang/Class;Ljava/lang/String;Ljava/lang/String;)Lmiui/reflect/Method;",
            (void*) com_miui_internal_os_Native_getMethodByClass},
    {"getMethod", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Lmiui/reflect/Method;",
            (void*) com_miui_internal_os_Native_getMethodByName},
    {"getMethod", "(Ljava/lang/Class;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ZLjava/lang/reflect/Method;)Lmiui/reflect/Method;",
            (void*) com_miui_internal_os_Native_getMethodByReflectMethod},
    {"getReflectMethod", "(Lmiui/reflect/Method;)Ljava/lang/reflect/Method;",
            (void*) com_miui_internal_os_Native_getReflectMethod},
    {"invoke", "(Lmiui/reflect/Method;Ljava/lang/Class;Ljava/lang/Object;[Ljava/lang/Object;)V",
            (void*) com_miui_internal_os_Native_invoke},
    {"invokeBoolean", "(Lmiui/reflect/Method;Ljava/lang/Class;Ljava/lang/Object;[Ljava/lang/Object;)Z",
            (void*) com_miui_internal_os_Native_invokeBoolean},
    {"invokeByte", "(Lmiui/reflect/Method;Ljava/lang/Class;Ljava/lang/Object;[Ljava/lang/Object;)B",
            (void*) com_miui_internal_os_Native_invokeByte},
    {"invokeChar", "(Lmiui/reflect/Method;Ljava/lang/Class;Ljava/lang/Object;[Ljava/lang/Object;)C",
            (void*) com_miui_internal_os_Native_invokeChar},
    {"invokeShort", "(Lmiui/reflect/Method;Ljava/lang/Class;Ljava/lang/Object;[Ljava/lang/Object;)S",
            (void*) com_miui_internal_os_Native_invokeShort},
    {"invokeInt", "(Lmiui/reflect/Method;Ljava/lang/Class;Ljava/lang/Object;[Ljava/lang/Object;)I",
            (void*) com_miui_internal_os_Native_invokeInt},
    {"invokeLong", "(Lmiui/reflect/Method;Ljava/lang/Class;Ljava/lang/Object;[Ljava/lang/Object;)J",
            (void*) com_miui_internal_os_Native_invokeLong},
    {"invokeFloat", "(Lmiui/reflect/Method;Ljava/lang/Class;Ljava/lang/Object;[Ljava/lang/Object;)F",
            (void*) com_miui_internal_os_Native_invokeFloat},
    {"invokeDouble", "(Lmiui/reflect/Method;Ljava/lang/Class;Ljava/lang/Object;[Ljava/lang/Object;)D",
            (void*) com_miui_internal_os_Native_invokeDouble},
    {"invokeObject", "(Lmiui/reflect/Method;Ljava/lang/Class;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;",
            (void*) com_miui_internal_os_Native_invokeObject},

    {"getConstructor", "(Ljava/lang/Class;Ljava/lang/String;)Lmiui/reflect/Constructor;",
            (void*) com_miui_internal_os_Native_getConstructorByClass},
    {"getConstructor", "(Ljava/lang/String;Ljava/lang/String;)Lmiui/reflect/Constructor;",
            (void*) com_miui_internal_os_Native_getConstructorByName},
    {"getConstructor", "(Ljava/lang/Class;Ljava/lang/String;Ljava/lang/String;Ljava/lang/reflect/Constructor;)Lmiui/reflect/Constructor;",
            (void*) com_miui_internal_os_Native_getConstructorByReflectConstructor},
    {"getReflectConstructor", "(Lmiui/reflect/Constructor;)Ljava/lang/reflect/Constructor;",
            (void*) com_miui_internal_os_Native_getReflectConstructor},
    {"newInstance", "(Lmiui/reflect/Constructor;[Ljava/lang/Object;)Ljava/lang/Object;",
            (void*) com_miui_internal_os_Native_newInstance},

    {"getField", "(Ljava/lang/Class;Ljava/lang/String;Ljava/lang/String;)Lmiui/reflect/Field;",
            (void*) com_miui_internal_os_Native_getFieldByClass},
    {"getField", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Lmiui/reflect/Field;",
            (void*) com_miui_internal_os_Native_getFieldByName},
    {"getField", "(Ljava/lang/Class;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ZLjava/lang/reflect/Field;)Lmiui/reflect/Field;",
            (void*) com_miui_internal_os_Native_getFieldByReflectField},
    {"getReflectField", "(Lmiui/reflect/Field;)Ljava/lang/reflect/Field;",
            (void*) com_miui_internal_os_Native_getReflectField},
    {"setFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;Z)V",
            (void*) com_miui_internal_os_Native_setBooleanFieldValue},
    {"setFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;B)V",
            (void*) com_miui_internal_os_Native_setByteFieldValue},
    {"setFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;C)V",
            (void*) com_miui_internal_os_Native_setCharFieldValue},
    {"setFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;S)V",
            (void*) com_miui_internal_os_Native_setShortFieldValue},
    {"setFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;I)V",
            (void*) com_miui_internal_os_Native_setIntFieldValue},
    {"setFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;J)V",
            (void*) com_miui_internal_os_Native_setLongFieldValue},
    {"setFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;F)V",
            (void*) com_miui_internal_os_Native_setFloatFieldValue},
    {"setFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;D)V",
            (void*) com_miui_internal_os_Native_setDoubleFieldValue},
    {"setFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;Ljava/lang/Object;)V",
            (void*) com_miui_internal_os_Native_setObjectFieldValue},
    {"getBooleanFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;)Z",
            (void*) com_miui_internal_os_Native_getBooleanFieldValue},
    {"getByteFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;)B",
            (void*) com_miui_internal_os_Native_getByteFieldValue},
    {"getCharFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;)C",
            (void*) com_miui_internal_os_Native_getCharFieldValue},
    {"getShortFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;)S",
            (void*) com_miui_internal_os_Native_getShortFieldValue},
    {"getIntFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;)I",
            (void*) com_miui_internal_os_Native_getIntFieldValue},
    {"getLongFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;)J",
            (void*) com_miui_internal_os_Native_getLongFieldValue},
    {"getFloatFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;)F",
            (void*) com_miui_internal_os_Native_getFloatFieldValue},
    {"getDoubleFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;)D",
            (void*) com_miui_internal_os_Native_getDoubleFieldValue},
    {"getObjectFieldValue", "(Lmiui/reflect/Field;Ljava/lang/Object;)Ljava/lang/Object;",
            (void*) com_miui_internal_os_Native_getObjectFieldValue},
};

int RegisterReflectNatives(JNIEnv *env, jclass clazz) {
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0]))) {
        ALOGE("RegisterNatives fialed for '%s' for reflect methods", JAVA_CLASS_NAME);
        return JNI_FALSE;
    }
    //init the mutex
    pthread_mutex_init(&sNativeMutex, NULL);
    return JNI_TRUE;
}

}

