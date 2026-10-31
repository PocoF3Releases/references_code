/********************************************************************
 Copyright (c) 2016 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 *********************************************************************/
#ifndef __MSFORWARDER_H
#define __MSFORWARDER_H

#include "minksocket.h"

#if defined (__cplusplus)
extern "C" {
#endif

typedef struct MSForwarder {
  int refs;
  int handle;
  MinkSocket *conn;
} MSForwarder;

Object MSForwarder_new(MinkSocket *conn, int handle);
MSForwarder *MSForwarderFromObject(Object obj);
/**
	Detach this MSForwarder from the remote handle and
	free its memory.
	Do not use this MSForwarder after calling detach.
 **/
int MSForwarder_detach(MSForwarder *me);

#if defined (__cplusplus)
}
#endif

#endif // __MSFORWARDER_H
