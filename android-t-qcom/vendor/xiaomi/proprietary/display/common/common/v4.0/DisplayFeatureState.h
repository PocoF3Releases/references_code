
#ifndef _DISPLAY_FEATURE_STATE_H_
#define _DISPLAY_FEATURE_STATE_H_

#include <utils/RefBase.h>
#include "OutputMethod.h"

namespace android {

class DisplayFeatureState: public RefBase
{
public:
    DisplayFeatureState(unsigned int displayId);
    virtual ~DisplayFeatureState();
    void init(bool support);
    virtual int output();
    virtual void setValue(struct OutputParam *pSOutputParams);
    int isSupport();
    bool getIsEnableByValue(int value);
    int getMethodByProduct();
    void setParam(void* params);
    virtual void dump(std::string& result);

protected:

    sp<OutputMethod> mOutPutMethod;
    int mFeatueId;
    OutputParam mParam;

private:
    bool mSupport;
    unsigned int mDisplayId;
};

class DebugMode: public DisplayFeatureState
{
public:
    DebugMode(unsigned int displayId);
    ~DebugMode();
};

class AdMode: public DisplayFeatureState
{
public:
    AdMode(unsigned int displayId);
    ~AdMode();
};

class SviMode: public DisplayFeatureState
{
public:
    SviMode(unsigned int displayId);
    ~SviMode();
};

class PanelMode: public DisplayFeatureState
{
public:
    PanelMode(unsigned int displayId);
    ~PanelMode();
};

class ColorMode: public DisplayFeatureState
{
public:
    ColorMode(unsigned int displayId);
    ~ColorMode();
};


class PccMode: public DisplayFeatureState
{
public:
    PccMode(unsigned int displayId);
    ~PccMode();
};

class  DSPPMode: public DisplayFeatureState
{
    public:
        DSPPMode(unsigned int displayId);
        ~DSPPMode();
};

class MTKMode: public DisplayFeatureState
{
public:
    MTKMode(unsigned int displayId);
    ~MTKMode();
};

class LtmMode: public DisplayFeatureState
{
public:
    LtmMode(unsigned int displayId);
    ~LtmMode();
};

class DumpMode: public DisplayFeatureState
{
public:
    DumpMode(unsigned int displayId);
    ~DumpMode();
};

class QDCMMode: public DisplayFeatureState
{
public:
    QDCMMode(unsigned int displayId);
    ~QDCMMode();
};
}; //namespace android
#endif //_DISPLAY_FEATURE_STATE_H_
