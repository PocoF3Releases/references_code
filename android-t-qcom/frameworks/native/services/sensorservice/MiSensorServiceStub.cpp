#include <iostream>
#include <dlfcn.h>
#include <log/log.h>
#include "MiSensorServiceStub.h"

void* MiSensorServiceStub::LibMiSensorServiceImpl = NULL;
IMiSensorServiceStub* MiSensorServiceStub::ImplInstance = NULL;
std::mutex MiSensorServiceStub::StubLock;

IMiSensorServiceStub* MiSensorServiceStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!ImplInstance) {
        LibMiSensorServiceImpl = dlopen(LIBPATH, RTLD_GLOBAL);
        if (LibMiSensorServiceImpl) {
            Create* create = (Create *)dlsym(LibMiSensorServiceImpl, "create");
            ALOGV("MiSensorServiceStub::GetImplInstance for debug create,%p", create);
            if (dlerror())
                ALOGE("MiSensorServiceStub::GetImplInstance dlerror %s", dlerror());
            else
                ImplInstance = create();
        }
    }
    return ImplInstance;
}

void MiSensorServiceStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibMiSensorServiceImpl) {
        Destroy* destroy = (Destroy *)dlsym(LibMiSensorServiceImpl, "destroy");
        destroy(ImplInstance);
        dlclose(LibMiSensorServiceImpl);
        LibMiSensorServiceImpl = NULL;
        ImplInstance = NULL;
    }
}

bool MiSensorServiceStub::isonwhitelist(String8 PackageName){
    bool boRet = false;
    IMiSensorServiceStub* Istub = GetImplInstance();
    if (Istub)
        boRet=Istub->isonwhitelist(PackageName);
    return boRet;

}

bool MiSensorServiceStub::isSensorDisableApp(const String8& packageName){
    bool boRet = false;
    IMiSensorServiceStub* Istub = GetImplInstance();
    if (Istub)
        boRet=Istub->isSensorDisableApp(packageName);
    return boRet;
}

void MiSensorServiceStub::setSensorDisableApp(const String8& packageName){
    IMiSensorServiceStub* Istub = GetImplInstance();
    if (Istub)
       Istub->setSensorDisableApp(packageName);
}

void MiSensorServiceStub::removeSensorDisableApp(const String8& packageName){
    IMiSensorServiceStub* Istub = GetImplInstance();
    if (Istub)
        Istub->removeSensorDisableApp(packageName);
}

String8 MiSensorServiceStub::getDumpsysInfo(){
    IMiSensorServiceStub* Istub = GetImplInstance();
    if (Istub)
       return Istub->getDumpsysInfo();
    return String8("");
}
void MiSensorServiceStub::SaveSensorUsageItem(int32_t sensorType, const String8& sensorName,
                                       const String8& packageName, bool activate) {
    IMiSensorServiceStub* Istub = GetImplInstance();
    if (Istub) {
        if (Istub->enableSensorEventToOnetrack()){
            Istub->SaveSensorUsageItem(sensorType, sensorName, packageName, activate);
        }
    }
}
void MiSensorServiceStub::handleProxUsageTime(const sensors_event_t& event, bool report){
    IMiSensorServiceStub* Istub = GetImplInstance();
    if (Istub) {
        if (Istub->enableSensorEventToOnetrack()){
            Istub->handleProxUsageTime(event, report);
        }
    }
}
void MiSensorServiceStub::handlePocModeUsageTime(const sensors_event_t& event, bool report){
    IMiSensorServiceStub* Istub = GetImplInstance();
    if (Istub) {
        if (Istub->enableSensorEventToOnetrack()){
            Istub->handlePocModeUsageTime(event, report);
        }
    }
}
void* MiSensorServiceStub:: timeToReport(void *arg) {
    IMiSensorServiceStub* Istub = GetImplInstance();
    if (Istub) {
        if (Istub->enableSensorEventToOnetrack()){
            Istub->timeToReport(arg);
        }
    }
    return arg;
}
void MiSensorServiceStub:: handleSensorEvent(const sensors_event_t& event) {
    IMiSensorServiceStub* Istub = GetImplInstance();
    if (Istub) {
        if (Istub->enableSensorEventToOnetrack()){
            Istub->handleSensorEvent(event);
        }
    }
}

void MiSensorServiceStub::dumpResultLocked(String8& result) {
    IMiSensorServiceStub* Istub = GetImplInstance();
    ALOGE("Istub is exist,%p", Istub);
    if (Istub)
        Istub->dumpResultLocked(result);
}
bool MiSensorServiceStub::appIsControlledLocked(uid_t uid, int32_t type) {
    bool boRet = false;
    IMiSensorServiceStub* Istub = GetImplInstance();
    if (Istub)
        boRet=Istub->appIsControlledLocked(uid,type);
    return boRet;
}

status_t MiSensorServiceStub::executeCommand(const sp<SensorService>& service, const Vector<String16>& args) {
    IMiSensorServiceStub* Istub = GetImplInstance();
    if (Istub)
       return Istub->executeCommand(service,args);
    return NO_ERROR;
}
