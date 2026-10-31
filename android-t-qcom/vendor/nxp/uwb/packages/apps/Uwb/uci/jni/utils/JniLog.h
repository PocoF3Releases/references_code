/*
*
* Copyright 2020 NXP.
*
* NXP Confidential. This software is owned or controlled by NXP and may only be
* used strictly in accordance with the applicable license terms. By expressly
* accepting such terms or by downloading,installing, activating and/or otherwise
* using the software, you are agreeing that you have read,and that you agree to
* comply with and are bound by, such license terms. If you do not agree to be
* bound by the applicable license terms, then you may not retain, install, activate
* or otherwise use the software.
*
*/

#ifndef _JNI_LOG_H_
#define _JNI_LOG_H_

#include <log/log.h>

/* global log level Ref */
extern bool uwb_debug_enabled;

static const char* UWB_JNI_LOG = "UwbJni";

#ifndef UNUSED
#define UNUSED(X) (void)X;
#endif

/* define log module included when compile */
#define ENABLE_JNI_LOGGING TRUE

/* ############## Logging APIs of actual modules ################# */
/* Logging APIs used by JNI module */
#if(ENABLE_JNI_LOGGING == TRUE)
    #define JNI_TRACE_D(...) {                                               \
        if (uwb_debug_enabled)                                               \
          LOG_PRI(ANDROID_LOG_DEBUG, UWB_JNI_LOG, __VA_ARGS__);          \
    }
    #define JNI_TRACE_I(...) {                                               \
        if (uwb_debug_enabled)                                               \
          LOG_PRI(ANDROID_LOG_INFO, UWB_JNI_LOG, __VA_ARGS__);           \
    }
    #define JNI_TRACE_W(...) {                                               \
        if (uwb_debug_enabled)                                               \
          LOG_PRI(ANDROID_LOG_WARN, UWB_JNI_LOG, __VA_ARGS__);           \
    }
    #define JNI_TRACE_E(...) {                                               \
        if (uwb_debug_enabled)                                               \
          LOG_PRI(ANDROID_LOG_ERROR, UWB_JNI_LOG, __VA_ARGS__);          \
    }
#else
    #define JNI_TRACE_D(...)
    #define JNI_TRACE_I(...)
    #define JNI_TRACE_W(...)
    #define JNI_TRACE_E(...)
#endif /* Logging APIs used by JNI module */

#endif /* _JNI_LOG_H_*/
