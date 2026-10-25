

#ifndef ANDROID_IMPL_FACTORY_H
#define ANDROID_IMPL_FACTORY_H
#include "Impl.h"
#include "PUBGMHDImpl.h"
#include "sgame/SGAMEImpl.h"
namespace android {

class ImplFactory {
public:
    enum GAME { SGAME  = 1, PUBGMHD = 2, NONE };

private:
    static SGAMEImpl sgameImpl;
    static PUBGMHDImpl pubgmhdImpl;
    static DefaultImplV2 defaultImplV2;

public:
    ImplFactory(/* args */);
    ~ImplFactory();
    static Impl* getImpl(GAME game);
};
} // namespace  android
#endif
