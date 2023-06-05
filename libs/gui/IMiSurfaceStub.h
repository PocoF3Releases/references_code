/*
 * Copyright (C) 2007 The Android Open Source Project
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


#ifndef ANDROID_IQCOM_SURFACE_IMPL_H
#define ANDROID_IQCOM_SURFACE_IMPL_H

#include <gui/Surface.h>

namespace android {

class IMiSurfaceStub {
public:
    IMiSurfaceStub() {}
    virtual ~IMiSurfaceStub() {}

    virtual void skipSlotRef(sp<Surface> surface, int i);
    virtual void doDynamicFps();
    virtual int doNotifyFbc(const int32_t& value);
    virtual int DoNotifyFbc(const int32_t& value, const uint64_t& bufID, const int32_t& param, const uint64_t& id);
    virtual bool supportFEAS();
};

typedef IMiSurfaceStub* Create();
typedef void Destroy(IMiSurfaceStub *);

} // namespace android

#endif
