/*
 * Copyright (C) 2013 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <private/android_filesystem_config.h>
#include "IDisplayFeatureControl.h"
#include <utils/Log.h>


#define LOG_TAG "DisplayFeatureControl"


namespace android {

class BpDisplayFeatureControl : public BpInterface<IDisplayFeatureControl>
{
public:

    BpDisplayFeatureControl(const sp<IBinder>& impl)
            : BpInterface<IDisplayFeatureControl>(impl)
    {
    }

    ~BpDisplayFeatureControl()
    {
    }

    virtual int setFeatureEnable(int displayId, int caseId, int modeId, int cookie)
    {
        Parcel data, reply;
        int result = -1;
        data.writeInterfaceToken(IDisplayFeatureControl::getInterfaceDescriptor());
        data.writeInt32(displayId);
        data.writeInt32(caseId);
        data.writeInt32(modeId);
        data.writeInt32(cookie);
        remote()->transact(SET_FEATURE_COMMOM, data, &reply);
        reply.readExceptionCode();
        result = reply.readInt32();
        return result;
    }

    virtual int setFunctionEnable(int displayId, int caseId, int modeId, int cookie)
    {
        Parcel data, reply;
        int result = -1;
        data.writeInterfaceToken(IDisplayFeatureControl::getInterfaceDescriptor());
        data.writeInt32(displayId);
        data.writeInt32(caseId);
        data.writeInt32(modeId);
        data.writeInt32(cookie);
        remote()->transact(caseId, data, &reply);
        reply.readExceptionCode();
        result = reply.readInt32();
        return result;
    }

    virtual void notify(int displayId, int caseId, int modeId, int cookie)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IDisplayFeatureControl::getInterfaceDescriptor());
        data.writeInt32(displayId);
        data.writeInt32(caseId);
        data.writeInt32(modeId);
        data.writeInt32(cookie);
        remote()->transact(NOTIFY, data, &reply, IBinder::FLAG_ONEWAY);
    }
};

IMPLEMENT_META_INTERFACE(DisplayFeatureControl, "miui.hareware.display.IDisplayFeatureService");

// ----------------------------------------------------------------------

status_t BnDisplayFeatureControl::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    // IPC should be from certain processes only
    IPCThreadState* ipc = IPCThreadState::self();
    const int callerPid = ipc->getCallingPid();
    const int callerUid = ipc->getCallingUid();
    const bool permission = (callerUid != AID_SHELL);
    if(!permission) {
        ALOGE("displayfeatureservice access denied: command=%d pid=%d uid=%d",
               code, callerPid, callerUid);
        return PERMISSION_DENIED;
    }
    switch (code) {
        case SET_FEATURE_COMMOM: {
            CHECK_INTERFACE(IDisplayFeatureControl, data, reply);
            data.readStrongBinder();
            int displayId = data.readInt32();
            int caseId = data.readInt32();
            int modeId = data.readInt32();
            int value = data.readInt32();
            int result = setFeatureEnable(displayId, caseId, modeId, value);
            reply->writeNoException();
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case SET_AD:
        case SET_PANEL_MODE:
        case SET_QUALCOMM_MODE:
        case SET_PCC_MODE:
        case SET_QC_DPPS_MODE:
        case SET_LTM:
        case SET_OPTION: {
            CHECK_INTERFACE(IDisplayFeatureControl, data, reply);
            data.readStrongBinder();
            int displayId = data.readInt32();
            int caseId = data.readInt32();
            int modeId = data.readInt32();
            int value = data.readInt32();
            int result = setFunctionEnable(displayId, caseId, modeId, value);
            reply->writeInt32(result);
            reply->writeNoException();
            return NO_ERROR;
        } break;
        case NOTIFY: {
            CHECK_INTERFACE(IDisplayFeatureControl, data, reply);
            data.readStrongBinder();
            int displayId = data.readInt32();
            int caseId = data.readInt32();
            int modeId = data.readInt32();
            int value = data.readInt32();
            notify(displayId, caseId, modeId, value);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

}
