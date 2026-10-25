#ifndef HRPD_EVDO_H
#define HRPD_EVDO_H

#include <jni.h>
#include <string.h>
#include "common.h"

int hrpd_access_auth(unsigned char *apdu,unsigned char *ss,unsigned char *out);

#endif
