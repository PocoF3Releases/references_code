
#ifndef _OUTPUT_METHOD_H_
#define _OUTPUT_METHOD_H_

#include <utils/RefBase.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <utils/Log.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#include "DisplayPostprocessing.h"
#include "DisplayFeatureHal.h"
#include <mi_display_device_manager.h>
#ifdef CWB_SUPPORT
#include "DisplayBufDump.h"
#endif
#define TEMP_BUILD

#include "DisplayColorManager_hidl.h"
#ifndef TEMP_BUILD
#include <vendor/display/postproc/1.0/IDisplayPostproc.h>
#include <vendor/display/postproc/1.0/types.h>
#endif


namespace android {

struct OutputParam {
    uint32_t function_id;
    uint32_t param1;
    uint32_t param2;
    uint32_t len;
    uint32_t *payload;
    std::vector<int32_t> vec_payload;
    std::vector<double> pcc_payload;
};

class OutputMethod: public RefBase
{
public:
    OutputMethod();
    virtual ~OutputMethod();
    virtual int output(OutputParam param);
    virtual int doDisable();
    virtual void setParam(void* params);
    virtual void dump(std::string& result);
};


class AdModeImpl: public OutputMethod
{
public:
    AdModeImpl(unsigned int display_id);
    ~AdModeImpl();
    virtual int output(OutputParam param);
    virtual void setParam(void* params);
    unsigned int mDisplayId = 0;
};

class SviModeImpl: public OutputMethod
{
public:
    SviModeImpl(unsigned int display_id);
    ~SviModeImpl();
    virtual int output(OutputParam param);
    virtual void setParam(void* params);
    unsigned int mDisplayId = 0;
};


class PanelModeImpl : public OutputMethod
{
public:
    PanelModeImpl(unsigned int display_id);
    ~PanelModeImpl();
    virtual int output(OutputParam param);
    virtual void setParam(void* params);
    unsigned int mDisplayId = 0;
    sp<DisplayDeviceManager> miDDM = nullptr;
};

class ColorModeImpl : public OutputMethod
{
public:
    ColorModeImpl(unsigned int display_id);
    ~ColorModeImpl();
    virtual int output(OutputParam param);
    virtual void setParam(void* params);
    unsigned int mDisplayId = 0;
    sp<DisplayColorManager> mDisplayColorManager;
};

class PccModeImpl : public OutputMethod
{
public:
    PccModeImpl(unsigned int display_id);
    ~PccModeImpl();
    virtual int output(OutputParam param);
    virtual void setParam(void* params);
    virtual void dump(std::string& result);
    unsigned int mDisplayId = 0;
    sp<DisplayPostprocessing> mDisplayPostProc;
};

class DSPPModeImpl : public OutputMethod
{
public:
    DSPPModeImpl(unsigned int display_id);
    ~DSPPModeImpl();
    virtual int output(OutputParam param);
    virtual void setParam(void* params);
    unsigned int mDisplayId = 0;
    sp<DisplayColorManager> mDisplayColorManager;
};

class MTKModeImpl : public OutputMethod
{
public:
    MTKModeImpl(unsigned int display_id);
    ~MTKModeImpl();
    virtual int output(OutputParam param);
    virtual void setParam(void* params);
    unsigned int mDisplayId = 0;
    sp<DisplayPostprocessing> mDisplayPostProc;
};

class LtmModeImpl: public OutputMethod
{
public:
    LtmModeImpl(unsigned int display_id);
    ~LtmModeImpl();
    virtual int output(OutputParam param);
    virtual void setParam(void* params);
    int ltmStatus = LTM_OFF;
    unsigned int mDisplayId = 0;
};

class BufDumpImpl: public OutputMethod
{
public:
    BufDumpImpl(unsigned int display_id);
    ~BufDumpImpl();
    virtual int output(OutputParam param);
    virtual void setParam(void* params);
    unsigned int mDisplayId = 0;
};
};

#endif
