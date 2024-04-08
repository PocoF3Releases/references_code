#ifndef ANDROID_CAMERA_IPRIVACYCAMERA_H
#define ANDROID_CAMERA_IPRIVACYCAMERA_H


#include <system/camera_metadata.h>

namespace android {
class IPrivacyCamera {
public:
    virtual ~IPrivacyCamera() { }
    virtual void saveFacesData(const camera_metadata_t *metadata) = 0;
    virtual void handleOutputBuffers(buffer_handle_t* outputBuffer) = 0;
};

} // end namespace android

#endif
