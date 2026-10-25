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

#include "IMiSurfaceStub.h"
#include <mutex>

namespace android {
//question: can't remove
class IMiSurfaceStub;

enum
{
    FBC_QUEUE_BEG,
    FBC_QUEUE_END,
    FBC_DEQUEUE_BEG,
    FBC_DEQUEUE_END,
    FBC_CNT,
    FBC_DISCNT,
};

class MiSurfaceStub {

private:

    MiSurfaceStub() {}
    static void *LibMiSurfaceImpl;
    static IMiSurfaceStub *ImplInstance;
    static IMiSurfaceStub *GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;

    static constexpr const char* LIBPATH = "libmigui.so";

public:
    virtual ~MiSurfaceStub() {}

    static void skipSlotRef(sp<Surface> surface, int i);

    static void doDynamicFps();

    static int doNotifyFbc(const int32_t& value);

    static int DoNotifyFbc(const int32_t& value, const uint64_t& bufID, const int32_t& param, const uint64_t& id);

    static bool supportFEAS();
};


} // namespace android
