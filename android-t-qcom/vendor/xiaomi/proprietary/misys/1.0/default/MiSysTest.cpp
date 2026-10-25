#include <utils/Log.h>
#include "MiSys.h"
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include<iostream>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "misys.test"

using namespace android;
using std::string;
using namespace::std;

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_vec;
using ::vendor::xiaomi::hardware::misys::V1_0::IMiSys;
using ::vendor::xiaomi::hardware::misys::V1_0::IResultValue;
using ::vendor::xiaomi::hardware::misys::V1_0::IFileListResult;
using ::vendor::xiaomi::hardware::misys::V1_0::IReadResult;

const hidl_string writeBuffer = "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";

int main(int /* argc */, char ** /* argv */) {
    IResultValue writeResult = IResultValue::MISYS_EINVAL;

    sp<IMiSys> service = IMiSys::getService();
    if (service == nullptr) {
      ALOGE("Unable to initialize the HIDL");
      return -1;
    }

    ALOGE("Initialize the misysHIDL successful");

    /**
     * test DirListFiles function
     * list the directory for "/mnt/vendor/persist"
     */
    service->DirListFiles("/mnt/vendor/persist", [&](auto& readResult){
      IResultValue value = readResult.value;
      uint32_t length = readResult.fileList.size();
      ALOGE("DirListFiles() readResult.size() = %u, value = %d\n", length, value);

      for ( uint32_t i = 0; i < length; i++) {
        ALOGE("DirListFiles return string(%d) value = %s\n",
          i, readResult.fileList[i].name.c_str());
      }
    });

     /**
     * test DirListFiles function
     * list the directory for "/mnt/vendor/persist/"
     */
    service->DirListFiles("/mnt/vendor/persist/", [&](auto& readResult){
      IResultValue value = readResult.value;
      uint32_t length = readResult.fileList.size();
      ALOGE("DirListFiles() readResult.size() = %u, value = %d\n", length, value);

      for ( uint32_t i = 0; i < length; i++) {
        ALOGE("DirListFiles return string(%d) value = %s\n",
          i, readResult.fileList[i].name.c_str());
      }
    });

    /**
     * test MiSysWriteFile function
     * write file "/mnt/vendor/persist/stability/" with slash
     * file name "MiSysWriteFile.txt"
     * with writeBuffer
     */
    writeResult = service->MiSysWriteFile("/mnt/vendor/persist/stability/",
                                          "MiSysWriteFile.txt",
                                          writeBuffer, writeBuffer.size(), 0);

    ALOGE("MiSysWriteFile result = %d for /mnt/vendor/persist/stability/MiSysWriteFile.txt\n",
          writeResult);

    writeResult = IResultValue::MISYS_EINVAL;

    /**
     * test MiSysWriteFile function
     * write file "/mnt/vendor/persist/stability" without slash
     * file name "MiSysWriteFileSlash.txt"
     * with writeBuffer
     */
    writeResult = IResultValue::MISYS_EINVAL;
    writeResult = service->MiSysWriteFile("/mnt/vendor/persist/stability/",
                                          "MiSysWriteFileSlash.txt",
                                          writeBuffer, writeBuffer.size(), 0);

    ALOGE("MiSysWriteFile result = %d for /mnt/vendor/persist/stability/MiSysWriteFileSlash.txt\n",  writeResult);


    /**
     * test function MiSysReadFile
     * read "/mnt/vendor/persist/stability/" with slash
     * file name "MiSysWriteFile.txt"
     */
    service->MiSysReadFile("/mnt/vendor/persist/stability/",
        "MiSysWriteFile.txt", [&](auto& readFileResult){
    ALOGE("MiSysReadFile return string value = \n");

    ALOGE("%s\n", readFileResult.data.c_str());

    ALOGE("MiSysReadFile return string value = from cout\n");

    cout << readFileResult.data.c_str() << endl;

    });

    /**
     * test function MiSysReadFile
     * read "/mnt/vendor/persist/stability/" without slash
     * file name "MiSysWriteFileSlash.txt"
     */
    service->MiSysReadFile("/mnt/vendor/persist/stability/",
        "qdss.config.sh", [&](auto& readFileResult){
    IResultValue value = readFileResult.value;
    ALOGE("MiSysReadFile() value = %d\n", value);

    ALOGE("MiSysReadFile return string value =\n");

    ALOGE("%s\n", readFileResult.data.c_str());

    ALOGE("MiSysReadFile return string value = from cout\n");

    cout << readFileResult.data.c_str() << endl;

    });

    /**
     * test function MiSysEraseFileOrDirectory
     * remove "/mnt/vendor/persist/stability/" with slash
     * file name "MiSysWriteFile.txt"
     */
    IResultValue eraseResult = IResultValue::MISYS_EINVAL;

    eraseResult = service->EraseFileOrDirectory("/mnt/vendor/persist/stability/", "MiSysWriteFile.txt");

    ALOGE("EraseFileOrDirectory result = %d\n",  eraseResult);

    /**
     * test function MiSysEraseFileOrDirectory
     * remove "/mnt/vendor/persist/stability/" without slash
     * file name "MiSysWriteFileSlash.txt"
     */
    eraseResult = service->EraseFileOrDirectory("/mnt/vendor/persist/stability", "MiSysWriteFileSlash.txt");

    ALOGE("EraseFileOrDirectory result = %d\n",  eraseResult);

    /**
     * test function MiSysEraseFileOrDirectory
     * remove directory "/mnt/vendor/persist/stability/"
     */

    writeResult = IResultValue::MISYS_EINVAL;
    writeResult = service->MiSysWriteFile("/mnt/vendor/persist/stability/",
                                          "MiSysWriteFile.txt",
                                          writeBuffer, writeBuffer.size(), 0);

    ALOGE("MiSysWriteFile result = %d for /mnt/vendor/persist/stability/MiSysWriteFile.txt\n",
          writeResult);

    writeResult = IResultValue::MISYS_EINVAL;

    writeResult = IResultValue::MISYS_EINVAL;
    writeResult = service->MiSysWriteFile("/mnt/vendor/persist/stability/",
                                          "MiSysWriteFileSlash.txt",
                                          writeBuffer, writeBuffer.size(), 0);

    ALOGE("MiSysWriteFile result = %d for /mnt/vendor/persist/stability/MiSysWriteFileSlash.txt\n",  writeResult);

    eraseResult = service->EraseFileOrDirectory("/mnt/vendor/persist/stability/", NULL);

    ALOGE("EraseFileOrDirectory result = %d\n",  eraseResult);

    return 0;
}
