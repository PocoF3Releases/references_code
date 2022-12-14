/*
 * used to verify OTA package
 *
 */

#include "otapackage_verifier.h"

/*
 * This function is used to test whether the signature of an OTA package is correct.
 *
 * parameter:
 *   otaPackagePath - The absolute path where the OTA package is located.
 *
 * return:
 *    0 - OTA package signature is correct
 *   -1 - OTA package signature is incorrect
 *   -2 - Failed to load device certificate
 *   -3 - Ota package path not found
 */
int
verify_otapackage(std::string otaPackagePath)
{
    ALOGI("Signature verification start");
    auto file_package = Package::CreateFilePackage(otaPackagePath, nullptr);
    static constexpr const char* CERTIFICATE_ZIP_FILE = "/system/etc/security/otacerts.zip";
    std::vector<Certificate> loaded_keys = LoadKeysFromZipfile(CERTIFICATE_ZIP_FILE);
    if (loaded_keys.empty()) {
        ALOGE("Failed to load keys");
        return -2;
    }
    ALOGI("%lu key(s) loaded from %s", loaded_keys.size(), CERTIFICATE_ZIP_FILE);
    if (file_package != nullptr) {
        Package* package = file_package.get();
        int err = verify_file(package, loaded_keys);
        if (err != VERIFY_SUCCESS) {
            ALOGE("Signature verification failed");
            return -1;
        } else {
            ALOGW("Signature Verification Successd!");
            return 0;
        }
    }
    ALOGE("package is null");
    return -3;
}

