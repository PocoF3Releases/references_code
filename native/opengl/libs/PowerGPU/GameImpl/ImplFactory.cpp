#include "ImplFactory.h"

#include "PUBGMHDImpl.h"
#include "sgame/SGAMEImpl.h"

namespace android {

SGAMEImpl ImplFactory::sgameImpl;
PUBGMHDImpl ImplFactory::pubgmhdImpl;
DefaultImplV2 ImplFactory::defaultImplV2;

ImplFactory::ImplFactory(/* args */) {}
ImplFactory::~ImplFactory() {}

Impl* ImplFactory::getImpl(GAME game) {
    switch (game) {
        case GAME::SGAME:
            return &sgameImpl;
            break;

        case GAME::PUBGMHD:
            return &pubgmhdImpl;
            break;
        default:
            return &defaultImplV2;
            break;
    }
}
} // namespace android
