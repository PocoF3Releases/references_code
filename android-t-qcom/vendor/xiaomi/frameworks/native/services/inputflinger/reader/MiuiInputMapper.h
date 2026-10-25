#ifndef ANDROID_MIUI_INPUT_MAPPER_H
#define ANDROID_MIUI_INPUT_MAPPER_H

#include "InputListener.h"
#include "utils/RefBase.h"
#include <utils/Mutex.h>
#include <utils/Looper.h>
#include <utils/Timers.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include "MiInputManager.h"

namespace android {
    enum {
        MODE_LEFT_SHOULDERKEY,
        MODE_RIGHT_SHOULDERKEY,
        MODE_MACRO,
    };

    class MiuiInputMapperPolicyInterface : public virtual RefBase {
        public:
            virtual void notifyTouchMotionEvent(const MotionEvent* motionEvent) = 0;
    };

    class MiuiInputMapper : public InputListenerInterface {
    public:
        MiuiInputMapper(InputListenerInterface& listener);
        void notifyConfigurationChanged(const NotifyConfigurationChangedArgs* args) override;
        void notifyKey(const NotifyKeyArgs* args) override;
        void notifyMotion(const NotifyMotionArgs* args) override;
        void notifySwitch(const NotifySwitchArgs* args) override;
        void notifySensor(const NotifySensorArgs* args) override;
        void notifyVibratorState(const NotifyVibratorStateArgs* args) override;
        void notifyDeviceReset(const NotifyDeviceResetArgs* args) override;
        void notifyPointerCaptureChanged(const NotifyPointerCaptureChangedArgs* args) override;

        void injectMotionEvent(const MotionEvent* event, int mode);

        void onConfigurationChanged(InputConfigTypeEnum type, int32_t changes);
        std::function<void(InputConfigTypeEnum, int32_t)> mOnConfigurationChangedListener =
                [this](InputConfigTypeEnum typeEnum, int32_t changes)
                {onConfigurationChanged(typeEnum, changes); };
        void setPolicy(const sp<MiuiInputMapperPolicyInterface>& policy);
        void dump(std::string& dump);

    protected:
        virtual ~MiuiInputMapper(){}

    private:
        struct MotionState {
            int32_t id;
            nsecs_t eventTime;

            int32_t deviceId;
            uint32_t source;
            int32_t displayId;
            uint32_t policyFlags;
            int32_t actionButton;
            int32_t flags;
            int32_t metaState;
            int32_t buttonState;

            MotionClassification classification;
            int32_t edgeFlags;

            int32_t actionMask;
            int32_t actionIndex;

            uint32_t pointerCount;
            PointerProperties pointerProperties[MAX_POINTERS];
            PointerCoords pointerCoords[MAX_POINTERS];
            float xPrecision;
            float yPrecision;

            float xCursorPosition;
            float yCursorPosition;
            nsecs_t downTime;
            nsecs_t readTime;
            std::vector<TouchVideoFrame> videoFrames;


            void dump(std::string& dump);
            void reset();

            void updateState(const MotionEvent* event, int mode, int seq);
            void updateState(const NotifyMotionArgs* args);
            int addPointer(PointerCoords coords, int32_t id);
            int adjustPointerId(int id, int mode);
            void publish(InputListenerInterface& listener);
        } mMotionState;

        std::mutex mLock;

        const IdGenerator mIdGenerator;
        InputListenerInterface& mListener;
        sp<MiuiInputMapperPolicyInterface> mPolicy;
    };

}//namespace:android
#endif