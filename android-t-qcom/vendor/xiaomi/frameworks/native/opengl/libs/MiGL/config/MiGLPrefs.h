#ifndef __MIGL_PREFS_H__
#define __MIGL_PREFS_H__

#include <binder/Parcelable.h>
#include <binder/Parcel.h>
#include <binder/ParcelFileDescriptor.h>
#include <GLES2/gl2.h>
#include "json/reader.h"

using namespace android;
using namespace android::os;

namespace com {
namespace xiaomi {
namespace joyose {

class MiGLPrefs {
    public:
        MiGLPrefs() = default;
        ~MiGLPrefs() = default;
        int readFromJoyose(std::string cmds);
        void queryParams(int features);
        Json::Value params;

        bool miglEnabled = false;

        bool modifyAF = false;
        int targetAFValue = 0;

        bool scaleResolutionSize = false;
        int targetWidthSize = 0;
        int targetHeightSize = 0;

        bool modifyShadingRate = false;
        int targetShadingRate = 0;

        int cacheGLValues = 0;

        bool enableYSSceneRecognize = false;
};

}
}
}

#endif
