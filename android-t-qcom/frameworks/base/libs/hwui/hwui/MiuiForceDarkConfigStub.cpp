#include "MiuiForceDarkConfigStub.h"

using namespace android;
void* MiuiForceDarkConfigStub::LibMiuiForceDarkConfigImpl = nullptr;
IMiuiForceDarkConfigStub* MiuiForceDarkConfigStub::ImplInstance = nullptr;
std::mutex MiuiForceDarkConfigStub::StubLock;
bool MiuiForceDarkConfigStub::inited = false;

IMiuiForceDarkConfigStub* MiuiForceDarkConfigStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!LibMiuiForceDarkConfigImpl && !inited) {
        LibMiuiForceDarkConfigImpl = dlopen(LIB_FORCEDARK_PATH, RTLD_LAZY);
        if (LibMiuiForceDarkConfigImpl) {
            Create* create = (Create *)dlsym(LibMiuiForceDarkConfigImpl, "createForceDark");
            if (create) {
                ImplInstance = create();
            } else {
                ALOGD("MiuiForceDarkConfigStub dlsym failed \"create\", reason:%s" , dlerror());
            }
        } else {
            ALOGD("MiuiForceDarkConfigStub open %s failed! dlopen - %s", LIB_FORCEDARK_PATH, dlerror());
        }
        inited = true;
    }
    return ImplInstance;
}

void MiuiForceDarkConfigStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibMiuiForceDarkConfigImpl) {
        Destroy* destroy = (Destroy *)dlsym(LibMiuiForceDarkConfigImpl, "destroyForceDark");
        if (destroy) {
            destroy(ImplInstance);
            dlclose(LibMiuiForceDarkConfigImpl);
            LibMiuiForceDarkConfigImpl = nullptr;
            ImplInstance = nullptr;
        } else {
            ALOGD("MiuiForceDarkConfigStub dlsym failed \"destroy\", reason:%s" , dlerror());
        }
    }
}

void MiuiForceDarkConfigStub::setConfig(float density, int mainRule, int secondaryRule,
                                        int tertiaryRule) {
    IMiuiForceDarkConfigStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGD("MiuiForceDarkConfigStub %s", __func__);
        Istub->setConfig(density, mainRule, secondaryRule, tertiaryRule);
    } else {
        ALOGD("MiuiForceDarkConfigStub implInstance not found");
    }
}

float MiuiForceDarkConfigStub::getDensity() {
    IMiuiForceDarkConfigStub* Istub = GetImplInstance();
    float ret = 0.0f;
    if (Istub) {
        ret = Istub->getDensity();
    }
    return ret;
}

int MiuiForceDarkConfigStub::getMainRule() {
    IMiuiForceDarkConfigStub* Istub = GetImplInstance();
    int ret = 0;
    if (Istub) {
        ret = Istub->getMainRule();
    }
    return ret;
}

int MiuiForceDarkConfigStub::getSecondaryRule() {
    IMiuiForceDarkConfigStub* Istub = GetImplInstance();
    int ret = 0;
    if (Istub) {
        ret = Istub->getSecondaryRule();
    }
    return ret;
}

int MiuiForceDarkConfigStub::getTertiaryRule() {
    IMiuiForceDarkConfigStub* Istub = GetImplInstance();
    int ret = 0;
    if (Istub) {
        ret = Istub->getTertiaryRule();
    }
    return ret;
}