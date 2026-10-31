#include <ACUtil.h>

#include <utils/Mutex.h>

#include <openssl/err.h>

#include "../common/log.h"


void ACUtil::logThreadErrors(__CALLER_DECLARE__) {

    static android::Mutex mutex;
    android::Mutex::Autolock autolock(mutex);

    ERR_load_crypto_strings();

    int e = 0;
    while ((e = ERR_get_error())) {
        char * str = ERR_error_string(e, NULL);
        LOGE("%s @ " __CALLER_FORMAT__ , str, __CALLER_USE__);
    }

    ERR_free_strings();
}
