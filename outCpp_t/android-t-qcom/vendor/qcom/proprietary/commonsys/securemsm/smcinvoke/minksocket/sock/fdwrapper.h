/********************************************************************
 Copyright (c) 2018 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 *********************************************************************/
#ifndef __FDWRAPPER_H
#define __FDWRAPPER_H

#if defined (__cplusplus)
extern "C" {
#endif

typedef struct FdWrapper {
  int refs;
  int handle;
  int descriptor;
} FdWrapper;

Object FdWrapper_new(int fd);
FdWrapper *FdWrapperFromObject(Object obj);

#if defined (__cplusplus)
}
#endif

#endif // __FdWrapper_H