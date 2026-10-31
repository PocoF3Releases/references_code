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


#ifndef ANDROID_TOUCH_INPUT_MAPPER_STUB_H
#define ANDROID_TOUCH_INPUT_MAPPER_STUB_H

#include "TouchInputMapper.h"

namespace android {
class IMiTouchInputMapperStub {
public:
    IMiTouchInputMapperStub() {};
    virtual ~IMiTouchInputMapperStub() {};
    virtual void init(TouchInputMapper* inputMapper);
    virtual bool dispatchMotionIntercept(NotifyMotionArgs* args, int surfaceOrientation);
};

typedef IMiTouchInputMapperStub* CreateMiTouchInputMapperStub();
typedef void DestroyMiTouchInputMapperStub(IMiTouchInputMapperStub *);
}
#endif