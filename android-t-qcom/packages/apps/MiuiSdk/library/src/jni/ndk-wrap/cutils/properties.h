#ifndef MIUI_CUTILS_PROPERTIES_H
#define MIUI_CUTILS_PROPERTIES_H

#ifdef BUILD_FOR_NDK

#include <stdlib.h>
#include <string.h>
#include <sys/system_properties.h>

#define PROPERTY_VALUE_MAX 256
static inline int property_get(const char* key, char* value, const char* default_value) {
    int len = __system_property_get(key, value);

    if (len > 0) {
        return len;
    }

    if (default_value) {
        len = strlen(default_value);
        memcpy(value, default_value, len+1);
    }
    return len;
}


static inline int property_set(const char* key, const char* value) {
    return -1; //not support
}

#endif
#endif
