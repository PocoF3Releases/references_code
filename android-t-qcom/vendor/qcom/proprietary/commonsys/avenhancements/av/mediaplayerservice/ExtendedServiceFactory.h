/*
 * Copyright (c) 2015,2021, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#ifndef _EXTENDED_SERVICE_FACTORY_H_
#define _EXTENDED_SERVICE_FACTORY_H_

namespace android {

using content::AttributionSourceState;

struct ExtendedServiceFactory : public AVMediaServiceFactory {
    ExtendedServiceFactory();

    virtual StagefrightRecorder *createStagefrightRecorder(const AttributionSourceState& attributionSource);

    virtual sp<ARTSPConnection> createARTSPConnection(bool uidValid, uid_t uid);
    virtual sp<ARTPConnection> createARTPConnection();

protected:
    virtual ~ExtendedServiceFactory();

private:
    ExtendedServiceFactory(const ExtendedServiceFactory &);
    ExtendedServiceFactory &operator=(ExtendedServiceFactory &);
};

extern "C" AVMediaServiceFactory *createExtendedMediaServiceFactory();

} //namespace android

#endif // _EXTENDED_SERVICE_FACTORY_H_

