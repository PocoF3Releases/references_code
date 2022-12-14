#ifndef DFC_NATIVE_SERVICE_H_
#define DFC_NATIVE_SERVICE_H_

#include <inttypes.h>
#include <unistd.h>

#include <vector>
#include <iostream>

#include <android-base/macros.h>
#include <binder/BinderService.h>
#include <cutils/multiuser.h>

#include "miui/dfc/BnDfc.h"

using namespace std;

namespace android {

class DfcNativeService : public BinderService<DfcNativeService>, public miui::dfc::BnDfc{
public:

    static DfcNativeService* getInstance() {
        if (sInstance == NULL) {
            sInstance = new DfcNativeService();
        }
        return sInstance;
    }


    static char const* getServiceName() { return "DfcNativeService"; }

/*  binder::Status Dfc_isSupport( bool* _aidl_return );
    binder::Status Dfc_getDFCVersion( int* _aidl_return );*/
    binder::Status Dfc_scanDuplicateFiles(const string& scanDuplicateFilesRules);
    binder::Status Dfc_getDuplicateFiles(const string& getDuplicateFilesRules, string* _aidl_return );
    binder::Status Dfc_compressDuplicateFiles( const string& compressDuplicateFilesRules, string* _aidl_return );
    binder::Status Dfc_restoreDuplicateFiles( const string& restoreDuplicateFilesRules, long* _aidl_return );
    binder::Status Dfc_forceStopScanDuplicateFiles();
    binder::Status Dfc_stopCompressDuplicateFiles();
    binder::Status Dfc_setDFCExceptRules(const string& exceptRules);

private:
    static DfcNativeService* sInstance;
};

} // namespace android

#endif