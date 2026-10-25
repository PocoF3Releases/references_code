/*
 *    Copyright 2021, Xiaomi Corporation All rights reserved.
 *
 *    Add sensorlist whitelist feature for idle task control
 *
 */

#include <utils/Log.h>
#include "MiSurfaceComposerImpl.h"
#include <gui/ISurfaceComposer.h>
namespace android {

void MiSurfaceComposerImpl::setMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays,
                              uint32_t flags, int64_t desiredPresentTime,const sp<IBinder>& remote,
                              const String16& interfaceDescriptor,uint32_t code)
{
     Parcel data, reply;
     data.writeInterfaceToken(interfaceDescriptor);
     data.writeUint32(static_cast<uint32_t>(miuiDisplays.size()));
     for (const auto& d : miuiDisplays) {
         d.write(data);
     }

     data.writeUint32(flags);
     data.writeInt64(desiredPresentTime);
     remote->transact(code, data, &reply);
}

status_t MiSurfaceComposerImpl::setFrameRateVideoToDisplay(uint32_t cmd, Parcel *data,
                               const String16& interfaceDescriptor, const sp<IBinder>& remote, uint32_t code)
{
    int err = NO_ERROR;
    Parcel data_rewrite, reply;

    data_rewrite.writeInterfaceToken(interfaceDescriptor);
    data_rewrite.writeInt32(data->readInt32());
    data_rewrite.writeInt32(data->readInt32());
    data_rewrite.writeUint32(data->readUint32());
    data_rewrite.writeFloat(data->readFloat());
    data_rewrite.writeBool(data->readBool());
    data_rewrite.writeUint32(data->readUint32());

    err = remote->transact(code, data_rewrite, &reply);
    return err;
}


extern "C" IMiSurfaceComposerStub* createMiSurfaceComposerImpl() {
    return new MiSurfaceComposerImpl;
}

extern "C" void destroyMiSurfaceComposerImpl(IMiSurfaceComposerStub* impl) {
    delete impl;
}

}

