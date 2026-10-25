
#include "OutputExtra.h"
#include <log/log.h>
namespace android {
namespace outputExtra {
void OutputExtra::setSequenceId(int sequenceId) {
    mSequenceId = sequenceId;
}
void OutputExtra::setDisplayState(bool isInternalDisplay) {
    mIsInternalDisplay = isInternalDisplay;
}
void OutputExtra::setLastFrameState(bool isLastFrameSecure) {
    mLastFrameSecure = isLastFrameSecure;
}
int OutputExtra::getSequenceId() const {
    return mSequenceId;
}
bool OutputExtra::isInternalDisplay() const {
    return mIsInternalDisplay;
}
bool OutputExtra::isLastFrameSecure() const {
    return mLastFrameSecure;
}

#ifdef MI_SF_FEATURE
void OutputExtra::setMiSecurityDisplayState(bool miSecurityDisplay) {
    mMiSecurityDisplay = miSecurityDisplay;
}
bool OutputExtra::getMiSecurityDisplayState() {
    return mMiSecurityDisplay;
}
#endif
#ifdef CURTAIN_ANIM
void OutputExtra::enableCurtainAnim(bool isEnable) {
    mEnableCurtainAnim = isEnable;
}

bool OutputExtra::isEnableCurtainAnim() {
    return mEnableCurtainAnim;
}

void OutputExtra::setCurtainAnimRate(float rate) {
    mCurtainAnimRate = rate;
}

float OutputExtra::getCurtainAnimRate() {
    return mCurtainAnimRate;
}
#endif

OutputExtra::OutputExtra() {
    if (DBG_OUTPUT_EXTRA) ALOGD("[OutputExtra] construct...");
}
OutputExtra::~OutputExtra() {
    if (DBG_OUTPUT_EXTRA) ALOGD("[OutputExtra] distroy...");
}
}  // namespace gamelayercontroller
}  // namespace android