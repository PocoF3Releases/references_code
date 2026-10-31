#pragma once
#include <aidl/android/hardware/biometrics/face/Error.h>
#include <aidl/android/hardware/biometrics/face/Feature.h>
#include <aidl/android/hardware/biometrics/face/AcquiredInfo.h>
#include <aidl/android/hardware/keymaster/HardwareAuthToken.h>
#include <hidl/HidlSupport.h>
#include <sys/endian.h>

namespace aidl::android::hardware::biometrics::face {

using aidl::android::hardware::biometrics::face::Error;
using aidl::android::hardware::biometrics::face::Feature;
using aidl::android::hardware::biometrics::face::AcquiredInfo;
using ::android::hardware::hidl_vec;
using aidl::android::hardware::keymaster::HardwareAuthToken;

class FaceUtils {
    public:
        static FaceUtils* getInstance();
        AcquiredInfo convert2FaceAcquiredInfo(int32_t acquiredInfo);
        Error convert2FaceError(int32_t error);
        Feature convert2FaceFeature(int32_t feature);
        uint32_t convertFeature2int(Feature feature);
        HardwareAuthToken hidlVec2AuthToken(const hidl_vec<uint8_t>& buffer);
        std::vector<uint8_t> authToken2AidlVec(const HardwareAuthToken& token);

        template <typename T, typename OutIter>
        inline OutIter copy_bytes_to_iterator(const T& value, OutIter dest) {
            const uint8_t* value_ptr = reinterpret_cast<const uint8_t*>(&value);
            return std::copy(value_ptr, value_ptr + sizeof(value), dest);
        }

        template <typename T, typename InIter>
        inline static InIter copy_bytes_from_iterator(T* value, InIter src) {
            uint8_t* value_ptr = reinterpret_cast<uint8_t*>(value);
            std::copy(src, src + sizeof(T), value_ptr);
            return src + sizeof(T);
        }
    private:
        FaceUtils();
        ~FaceUtils();
        //用于保证同一时间只有一个对象访问smc
        //std::mutex smc_mutex;
};

}