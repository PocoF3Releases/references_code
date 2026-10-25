#ifndef OTAPACKAGE_VERIFIER_H_
#define OTAPACKAGE_VERIFIER_H_

#define LOG_TAG "OTA_VERIFIER"

#include <cutils/log.h>
#include <utils/Log.h>
#include <iostream>
#include <vector>
#include <string>
#include "install/package.h"
#include "install/verifier.h"

int verify_otapackage(std::string otaPackagePath);

#endif /* OTAPACKAGE_VERIFIER_H_ */