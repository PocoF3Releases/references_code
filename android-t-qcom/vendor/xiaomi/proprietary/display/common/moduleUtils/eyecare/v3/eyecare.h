#ifndef _EYECARE_H_
#define _EYECARE_H_
#include <iostream>
#include <stdio.h>
#include <string>
#include <math.h>
#include <algorithm>
#include <ctime>
#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <fstream>
#include <cutils/properties.h>
#include <unistd.h>
#include <pthread.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include "DisplayFeatureHal.h"
#include "display_utils.h"
#include "parseXml.h"
#define EYECARE_START_LEVEL 58
#define EYECARE_PCC_LEN 9
namespace android {
typedef struct PccEyeCareParams {
    double Coeffs[9][4];
    int maxCCT;
    int minCCT;
} PccEyeCareParams;

class EyeCare : public RefBase
{
public:
    EyeCare(int display_id);
    virtual ~EyeCare() { };
    int getEyeCareCoeff(int level, double* coeff);

private:
    void init();
    PccEyeCareParams mParam;
    bool mInit = false;
    int displayId = DISPLAY_PRIMARY;
    sp<MiParseXml> mParseXml[DISPLAY_MAX];
};
}
#endif