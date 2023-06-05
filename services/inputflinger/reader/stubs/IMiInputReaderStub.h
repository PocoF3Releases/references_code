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


#ifndef ANDROID_INPUTREADER_STUB_H
#define ANDROID_INPUTREADER_STUB_H

#include "InputReader.h"

namespace android {
class IMiInputReaderStub {
public:
    IMiInputReaderStub() {};
    virtual ~IMiInputReaderStub() {};
    virtual void init(InputReader* inputReader);
    virtual bool getInputReaderAll();
    virtual void addDeviceLocked(std::shared_ptr<InputDevice> inputDevice);
    virtual void removeDeviceLocked(std::shared_ptr<InputDevice> inputDevice);
    virtual bool handleConfigurationChangedLockedIntercept(nsecs_t when);
    virtual void loopOnceChanges(uint32_t changes);
};

typedef IMiInputReaderStub* Create();
typedef void Destroy(IMiInputReaderStub *);
}
#endif