#include <unistd.h>
#include "aapt_hook.h"

bool isValidMccLength(size_t len) {
    return len == 3 || len == 4;
}

bool isValidMncLength(size_t len) {
    return len != 0 && len <= 4;
}

