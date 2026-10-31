#ifndef MI_INPUT_MANAGER_H
#define MI_INPUT_MANAGER_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <stdint.h>
#include <errno.h>
#include <binder/Parcel.h>
#include <private/gui/ParcelUtils.h>
#include <cutils/properties.h>
#include <android-base/strings.h>
#include "InputDebugConfig.h"
#include "InputCommonConfig.h"
#include "InputKnockConfig.h"
#include <functional>
#include <vector>
#include <input/Input.h>

namespace android {

/*
java层向native层下发不同的Config，在setInputConfig里进行解析对应不同的InputConfig，有对应且解析成功之后返回SUCCESS，之后可以将之前的
InputConfig和最新的InputConfig进行diff，得到当前changed的属性，并不是所有的属性都需要知道change，需要change的都要单独定义
*/

//不同Config的Type，根据type进行对应解析
enum InputConfigTypeEnum {
    INPUT_CONFIG_TYPE_ERROR = -1,
    INPUT_CONFIG_NO_CHANGE = 0,
    INPUT_CONFIG_TYPE_DEBUG_CONFIG = 0x01,
    INPUT_CONFIG_TYPE_COMMON_CONFIG = 0x02,
    INPUT_CONFIG_TYPE_KNOCK_CONFIG = 0x03,
};

class MiInputManager : public RefBase {
public:
    //Debug配置
    sp<InputDebugConfig> mInputDebugConfig;
    //指关节配置
    //键鼠配置
    //通用配置
    sp<InputCommonConfig> mInputCommonConfig;
    //叩击配置
    sp<InputKnockConfig> mInputKnockConfig;

    std::vector<std::function<void(InputConfigTypeEnum, int32_t)>> mOnConfigurationChangedListeners;

    //result:没有匹配或者解析错误INPUT_CONFIG_TYPE_ERROR，解析成功返回changes，changes可能是0
    virtual int32_t setInputConfig(int32_t, Parcel* parcel);

    static MiInputManager* getInstance();
    virtual bool getInputDispatcherMajor();
    virtual bool getInputDispatcherDetail();
    virtual bool getInputDispatcherAll();
    virtual bool getInputReaderAll();
    virtual bool getInputTransportAll();
    virtual bool getSynergyMode();
    virtual bool isInputMethodShown();
    virtual bool isCustomizeInputMethod();
    virtual bool isPadMode();
    virtual bool isOnewayMode();

    virtual void dumpMiInputManager(std::string& dump);
    virtual void addOnConfigurationChangedListener(
            const std::function<void(InputConfigTypeEnum, int32_t)>& onConfigurationChangedListener);
    virtual void onConfigurationChanged(InputConfigTypeEnum type, int32_t changes);

    virtual void injectMotionEvent(const MotionEvent* event, int mode);
private:
    MiInputManager();
    virtual ~MiInputManager() {}
    static MiInputManager* sImplInstance;
    static std::mutex mInstLock;
    static std::mutex mIistenerLock;
};

}

#endif // MI_INPUT_MANAGER_H