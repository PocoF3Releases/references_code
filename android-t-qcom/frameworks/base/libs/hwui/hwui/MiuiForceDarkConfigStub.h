#ifndef ANDROID_MIUI_FORTH_DARK_STUB_H
#define ANDROID_MIUI_FORTH_DARK_STUB_H

#include "IMiuiForceDarkConfigStub.h"
#include <mutex>
#include <dlfcn.h>
#include <iostream>
#include <log/log.h>

#if defined(__LP64__)
#define LIB_FORCEDARK_PATH "/system_ext/lib64/libforcedarkimpl.so"
#else
#define LIB_FORCEDARK_PATH "/system_ext/lib/libforcedarkimpl.so"
#endif

namespace android {

class MiuiForceDarkConfigStub {
private:
    MiuiForceDarkConfigStub() {}
    static void *LibMiuiForceDarkConfigImpl;
    static IMiuiForceDarkConfigStub *ImplInstance;
    static IMiuiForceDarkConfigStub *GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;
    static bool inited;

public:
    virtual ~MiuiForceDarkConfigStub() {}
    static void setConfig(float density, int mainRule, int secondaryRule, int tertiaryRule);
    static float getDensity();
    static int getMainRule();
    static int getSecondaryRule();
    static int getTertiaryRule();
    enum MainRule {
        RULE_DEPRECATED = 1 << 0,
        RULE_BITMAP_PALETTE_COMPUTION = 1 << 1,
        RULE_BACKGROUND_DRAWABLE = 1 << 2,
        RULE_ALWAYS_ALLOW_FOR_TEXTVIEW = 1 << 3,
        RULE_ALWAYS_LIGHT_TEXT = 1 << 4,
        RULE_RES_LOADED_DRAWABLE = 1 << 5,
        RULE_FOREGROUND_FOR_BACKGROUND_DRAWABLE = 1 << 6,
        RULE_ALWAYS_ALLOW_FOR_VIEW = 1 << 7,
        RULE_INVERT_EXCLUDE_BITMAP = 1<< 8,
        RULE_FORCE_DARK_SOFTWARE = 1<< 9,
        RULE_NONE_ASSET_DRAWABLE_NO_FORCE_DARK = 1 << 10,
    };

    enum SecondaryRule {
        RULE_ONE_CHILD_WIDTH_WHOLE = 1 << 0,
        RULE_ONE_CHILD_WIDTH_HALF = 1 << 1,
        RULE_ZERO_CHILD_EMPTY_VIEW = 1 << 2,
        RULE_SKIP_FOR_UNKNOWN = 1 << 3,
    };

protected:
    float mDensity;
    int mMainRule;
    int mSecondaryRule;
    int mTertiaryRule;
};

}
#endif // ANDROID_MIUI_FORTH_DARK_STUB_H
