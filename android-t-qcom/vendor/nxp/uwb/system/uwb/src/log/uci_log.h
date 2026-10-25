/*
*
* Copyright 2018-2019 NXP.
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

#ifndef _UCI_LOG_H_
#define _UCI_LOG_H_

#include <log/log.h>

/* global log level Ref */
extern bool uwb_debug_enabled;

static const char* UWB_UCI_CORE_LOG = "UwbUciCore";

#ifndef UNUSED
#define UNUSED(X) (void)X;
#endif

/* define log module included when compile */
#define ENABLE_UCI_LOGGING TRUE

/* ############## Logging APIs of actual modules ################# */
/* Logging APIs used by UCI module */
#if(ENABLE_UCI_LOGGING == TRUE)
    #define UCI_TRACE_D(...) {                                               \
        if (uwb_debug_enabled)                                               \
          LOG_PRI(ANDROID_LOG_DEBUG, UWB_UCI_CORE_LOG, __VA_ARGS__);         \
    }
    #define UCI_TRACE_I(...) {                                               \
        if (uwb_debug_enabled)                                               \
          LOG_PRI(ANDROID_LOG_INFO, UWB_UCI_CORE_LOG, __VA_ARGS__);          \
    }
    #define UCI_TRACE_W(...) {                                               \
        if (uwb_debug_enabled)                                               \
          LOG_PRI(ANDROID_LOG_WARN, UWB_UCI_CORE_LOG, __VA_ARGS__);          \
    }
    #define UCI_TRACE_E(...) {                                               \
        if (uwb_debug_enabled)                                               \
          LOG_PRI(ANDROID_LOG_ERROR, UWB_UCI_CORE_LOG, __VA_ARGS__);         \
    }
#else
    #define UCI_TRACE_D(...)
    #define UCI_TRACE_I(...)
    #define UCI_TRACE_W(...)
    #define UCI_TRACE_E(...)
#endif /* Logging APIs used by UCI module */

#endif /* NXPLOG__H_INCLUDED */
