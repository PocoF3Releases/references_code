#ifndef SERVICES_SURFACEFLINGER_OUTPUT_EXTRA_H_
#define SERVICES_SURFACEFLINGER_OUTPUT_EXTRA_H_

#define DBG_OUTPUT_EXTRA                     0

#include <math/vec4.h>
#include <ui/GraphicTypes.h>

namespace android {

namespace outputExtra {

class OutputExtra {
 public:
    explicit OutputExtra();
    ~OutputExtra();

    int getSequenceId() const;
    bool isInternalDisplay() const;
    bool isLastFrameSecure() const;
    void setSequenceId(int sequenceId);
    void setDisplayState(bool isInternalDisplay);
    void setLastFrameState(bool isLastFrameSecure);
#ifdef CURTAIN_ANIM
    void enableCurtainAnim(bool isEnable);
    bool isEnableCurtainAnim();
    void setCurtainAnimRate(float rate);
    float getCurtainAnimRate();
#endif

#ifdef MI_SF_FEATURE
    void setMiSecurityDisplayState(bool miSecurityDisplay);
    bool getMiSecurityDisplayState();
#endif
#ifdef MI_SF_FEATURE
    // HDR Dimmer
    void enableHdrDimmer(bool enable, float factor) {
        mHdrDimmerEnabled = enable; mHdrDimmerFactor = factor; }
    bool isHdrDimmerEnabled() { return mHdrDimmerEnabled; }
    float getHdrDimmerFactor() { return mHdrDimmerFactor; }
    void setColorScheme(int colorScheme) { mColorScheme = colorScheme; }
    int getColorScheme() { return mColorScheme; }
    // END
#endif
 private:
    int mSequenceId = -1;
    bool mIsInternalDisplay = true;
    bool mLastFrameSecure = false;

#ifdef MI_SF_FEATURE
    bool mMiSecurityDisplay = false;
    // Hdr Dimmer
    bool mHdrDimmerEnabled = false;
    float mHdrDimmerFactor = 1.f;
    int mColorScheme = 0;
    // END
#endif
#ifdef CURTAIN_ANIM
    bool mEnableCurtainAnim = false;
    float mCurtainAnimRate = -1.f;
#endif
};

}  // namespace outputExtra

}  // namespace android

#endif  // SERVICES_SURFACEFLINGER_OUTPUT_EXTRA_H_
