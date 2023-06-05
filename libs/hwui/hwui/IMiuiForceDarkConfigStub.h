#ifndef ANDROID_IMIUI_FORTH_DARK_STUB_H
#define ANDROID_IMIUI_FORTH_DARK_STUB_H

namespace android {

class IMiuiForceDarkConfigStub {
public:
    virtual ~IMiuiForceDarkConfigStub() {};
    virtual void setConfig(float density, int mainRule, int secondaryRule, int tertiaryRule);
    virtual float getDensity();
    virtual int getMainRule();
    virtual int getSecondaryRule();
    virtual int getTertiaryRule();
};

typedef IMiuiForceDarkConfigStub* Create();
typedef void Destroy(IMiuiForceDarkConfigStub *);

}
#endif // ANDROID_IMIUI_FORTH_DARK_STUB_H
