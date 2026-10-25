#ifndef FBO_NATIVE_H_
#define FBO_NATIVE_H_

#include <inttypes.h>
#include <unistd.h>

#include <vector>
#include <unordered_map>

#include <android-base/macros.h>
#include <binder/BinderService.h>
#include <cutils/multiuser.h>

#include "miui/fbo/BnFbo.h"

namespace android {

class FboNativeService : public BinderService<FboNativeService>, public miui::fbo::BnFbo{
public:

    static FboNativeService* getInstance() {
        if (sInstance == NULL) {
            sInstance = new FboNativeService();
        }
        return sInstance;
    }


    static char const* getServiceName() { return "FboNativeService"; }
    
    binder::Status FBO_isSupport( bool* _aidl_return );
    binder::Status FBO_trigger( const std::string& path );
    binder::Status FBO_stateCtl( const std::string& cmd, std::string* _aidl_return );
//    binder::Status FBO_notifyFragStatus( std::string& _aidl_return );

private:
    static FboNativeService* sInstance;
};

} // namespace android

#endif