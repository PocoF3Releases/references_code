#ifndef __MIGL_SETTINGS_H__
#define __MIGL_SETTINGS_H__

#include <GLES2/gl2.h>
#include <utils/String8.h>
#include "../config/MiGLPrefs.h"

using namespace android;
using namespace com::xiaomi::joyose;

namespace android {
namespace migl {

class MiGLSettings {
    private:
        MiGLSettings();
        int initMiGLPrefs();
        void onRemoteServiceDied();

        unsigned int myPid;
        String8 mAppName;
        bool initialized = false;
        sp<IBinder> remoteService;
        MiGLPrefs* miglPrefs;
    public:
        ~MiGLSettings() {
            delete miglPrefs;
        }

        inline bool isMiGLEnabled() {
            if (miglPrefs == nullptr)
                return false;
            return miglPrefs->miglEnabled;
        }

        inline bool modifyAF() {
            if (miglPrefs == nullptr)
                return false;
            return miglPrefs->modifyAF;
        }
        inline GLsizei targetAFValue() {
            if (miglPrefs == nullptr) return 0;
            return (GLsizei)(miglPrefs->targetAFValue);
        }

        inline bool scaleResolutionSize() {
            if (miglPrefs == nullptr)
                return false;
            return miglPrefs->scaleResolutionSize;
        }
        inline GLsizei targetWidthSize() {
            if (miglPrefs == nullptr) return 0;
            return (GLsizei)(miglPrefs->targetWidthSize);
        }
        inline GLsizei targetHeightSize() {
            if (miglPrefs == nullptr) return 0;
            return (GLsizei)(miglPrefs->targetHeightSize);
        }

        inline bool modifyShadingRate() {
            if (miglPrefs == nullptr)
                return false;
            return miglPrefs->modifyShadingRate;
        }
        inline int targetShadingRate() {
            if (miglPrefs == nullptr) return 0;
            return miglPrefs->targetShadingRate;
        }

        inline bool cacheGLValues() {
            if (miglPrefs == nullptr)
                return false;
            return miglPrefs->cacheGLValues;
        }

        inline bool isEnableYSSceneRecognize() {
            if (miglPrefs == nullptr)
                return false;
            return miglPrefs->enableYSSceneRecognize;
        }

        static MiGLSettings& getInstance() {
            static MiGLSettings instance;
            return instance;
        }
};

}
}

#endif
