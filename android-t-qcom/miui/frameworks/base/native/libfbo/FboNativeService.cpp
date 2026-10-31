#include "FboNativeService.h"

extern bool FBO_F2FS_isSupport();
extern void FBO_F2FS_Trigger(const std::string& path);
extern std::string FBO_Native_State(const std::string& cmd);
namespace android {

FboNativeService* FboNativeService::sInstance = NULL;

binder::Status FboNativeService::FBO_isSupport( bool* _aidl_return ) {

    *_aidl_return = FBO_F2FS_isSupport();

    return binder::Status::ok();
}

binder::Status FboNativeService::FBO_trigger( const std::string& path ){

    FBO_F2FS_Trigger(path);

    return binder::Status::ok();
}

binder::Status FboNativeService::FBO_stateCtl( const std::string& cmd, std::string* _aidl_return ){

    * _aidl_return = FBO_Native_State(cmd);

    return binder::Status::ok();
}

} //android