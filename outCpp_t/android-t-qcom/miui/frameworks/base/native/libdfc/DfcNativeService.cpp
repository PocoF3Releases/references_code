#include "DfcNativeService.h"

/*
extern bool Dfc_isSupportImp();
extern int Dfc_getDFCVersionImp();
*/
extern void Dfc_scanDuplicateFilesImp(const string& scanDuplicateFilesRules);
extern string Dfc_getDuplicateFilesImp(const string& getDuplicateFilesRules);
extern string Dfc_compressDuplicateFilesImp(const string& compressDuplicateFilesRules);
extern long Dfc_restoreDuplicateFilesImp(const string& restoreDuplicateFilesRules);
extern void Dfc_forceStopScanDuplicateFilesImp();
extern void Dfc_stopCompressDuplicateFilesImp();
extern void Dfc_setDFCExceptRulesImp(const string& exceptRules);

namespace android {

DfcNativeService* DfcNativeService::sInstance = NULL;

/*
binder::Status DfcNativeService::Dfc_isSupport(bool* _aidl_return){

    *_aidl_return = Dfc_isSupportImp();

    return binder::Status::ok();
}

binder::Status DfcNativeService::Dfc_getDFCVersion( int* _aidl_return ){

    *_aidl_return = Dfc_getDFCVersionImp();

    return binder::Status::ok();
}
*/

binder::Status DfcNativeService::Dfc_scanDuplicateFiles(const string& scanDuplicateFilesRules){

    Dfc_scanDuplicateFilesImp(scanDuplicateFilesRules);

    return binder::Status::ok();
}

binder::Status DfcNativeService::Dfc_getDuplicateFiles(const string& getDuplicateFilesRules, string* _aidl_return ){

    * _aidl_return = Dfc_getDuplicateFilesImp(getDuplicateFilesRules);

    return binder::Status::ok();
}

binder::Status DfcNativeService::Dfc_compressDuplicateFiles( const string& compressDuplicateFilesRules, string* _aidl_return ){

    * _aidl_return = Dfc_compressDuplicateFilesImp(compressDuplicateFilesRules);

    return binder::Status::ok();
}

binder::Status DfcNativeService::Dfc_restoreDuplicateFiles( const string& restoreDuplicateFilesRules, long* _aidl_return ){

    * _aidl_return = Dfc_restoreDuplicateFilesImp(restoreDuplicateFilesRules);

    return binder::Status::ok();
}

binder::Status DfcNativeService::Dfc_forceStopScanDuplicateFiles(){

    Dfc_forceStopScanDuplicateFilesImp();

    return binder::Status::ok();
}

binder::Status DfcNativeService::Dfc_stopCompressDuplicateFiles(){

    Dfc_stopCompressDuplicateFilesImp();

    return binder::Status::ok();
}

binder::Status DfcNativeService::Dfc_setDFCExceptRules(const string& exceptRules){

    Dfc_setDFCExceptRulesImp(exceptRules);

    return binder::Status::ok();
}

} //android