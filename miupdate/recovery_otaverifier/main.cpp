/*
 * used to test the librecovery_otaverifier library
 *
 */

#include <cutils/log.h>
#include <utils/Log.h>
#include "otapackage_verifier.h"

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        ALOGW("need parameter");
        return -1;
    }
    ALOGW("start test");
    int result = verify_otapackage(std::string(argv[1]));
    ALOGW("test end , result = %d", result);
    return 0;
}
