/********************************************************************
 Copyright (c) 2016 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 *********************************************************************/
#ifndef __MINKSOCKET_H
#define __MINKSOCKET_H

#include <pthread.h>
#include <stdbool.h>
#include "object.h"


#if defined (__cplusplus)
extern "C" {
#endif

extern uintptr_t gMinkPeerUIDTLSKey;
extern uintptr_t gMinkPeerGIDTLSKey;

typedef struct MinkSocket MinkSocket;

MinkSocket *MinkSocket_new(Object opener);
void MinkSocket_retain(MinkSocket *me);
void MinkSocket_release(MinkSocket *me);
void MinkSocket_registerForwarder(MinkSocket *me);
void MinkSocket_unregisterForwarder(MinkSocket *me);
int MinkSocket_detach(MinkSocket *me);
/** MinkSocket_attachObject attempts to attach an object
    with a specified handle much like MinkIPC_Connect does with an opener.
 **/
void MinkSocket_attachObject(MinkSocket *me, int handle, Object *obj);
int MinkSocket_detachObject(Object *obj);
int32_t MinkSocket_invoke(MinkSocket *me, int32_t h,
                  ObjectOp op, ObjectArg *args, ObjectCounts k);
int32_t MinkSocket_sendClose(MinkSocket *me, int handle);
void *MinkSocket_dispatch(void *me);
void MinkSocket_start(MinkSocket *me, int sock);
void MinkSocket_close(MinkSocket *me, int32_t err);
void MinkSocket_delete(MinkSocket *me);
bool MinkSocket_isConnected(MinkSocket *me);

#if defined (__cplusplus)
}
#endif

#endif //__MINKSOCKET_H
