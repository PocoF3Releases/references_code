/********************************************************************
 Copyright (c) 2016 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 *********************************************************************/
#ifndef __THREADPOOL_H
#define __THREADPOOL_H

#include "qlist.h"
#include <stdbool.h>

#define THREADPOOL_MAX_THREADS 2

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ThreadPool ThreadPool;

typedef void *(*ThreadWorkFunc)(void *);

typedef struct ThreadWork {
   QNode n;
   ThreadWorkFunc workFunc;
   void *args;
} ThreadWork;

static inline void ThreadWork_init(ThreadWork *w,
                                   ThreadWorkFunc func,
                                   void *args) {
  QNode_construct(&w->n);
  w->workFunc = func;
  w->args = args;
}

ThreadPool *ThreadPool_new(void);
void ThreadPool_retain(ThreadPool *me);
void ThreadPool_release(ThreadPool *me);

//wait for all threads to exit gracefully
void ThreadPool_wait(ThreadPool *me);
void ThreadPool_stop(ThreadPool *me);
void ThreadPool_queue(ThreadPool *me, ThreadWork *work);

#ifdef __cplusplus
}
#endif
#endif /* __THREADPOOL_H */
