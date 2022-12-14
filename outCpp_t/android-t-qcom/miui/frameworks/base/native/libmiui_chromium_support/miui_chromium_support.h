#ifndef MIUI_CHROMIUM_SUPPORT_H
#define MIUI_CHROMIUM_SUPPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>

// This header/structure is conform to Android L.

static const int kMcsDrawGLInfoVersion = 1;

struct McsDrawGLInfo {
  // TODO: Disable version in this release.
  // int version;

  enum Mode {
    kModeDraw,
    kModeProcess,
    kModeProcessNoContext,
    kModeSync,
  } mode;

  int clip_left;
  int clip_right;
  int clip_top;
  int clip_bottom;

  int width;
  int height;

  bool is_layer;

  float transform[16];

  // The following fields are for Android 4.x.
  enum StatusMask {
    kStatusMaskDone = 0x0,
    kStatusMaskDraw = 0x1,
    kStatusMaskInvoke = 0x2,
  };

  unsigned int status_mask;

  float dirty_left;
  float dirty_top;
  float dirty_right;
  float dirty_bottom;
};

// TODO(shouqun): The third parameter 'spare' is not defined here. It's passed
// to chromium in Android L although not used. For current x9 release, we will
// still use two parameters for this callback definition, and will consider add
// the third parameter after x9 release.
typedef void McsDrawGLCallback(long context, McsDrawGLInfo* draw_info/*, void* spare*/);

enum McsMapMode {
  MCS_MAP_READ_ONLY,
  MCS_MAP_WRITE_ONLY,
  MCS_MAP_READ_WRITE,
};


typedef void* FnMcsGLFunctorCreate(void* context);
typedef void  FnMcsGLFunctorDestroy(void* functor);
typedef void  FnMcsGLFunctorSetCallback(McsDrawGLCallback callback);

typedef void* FnMcsGraphicBufferCreate(int w, int h);
typedef void  FnMcsGraphicBufferRelease(void* buffer);
typedef int   FnMcsGraphicBufferMap(void* buffer, McsMapMode mode, void** vaddr);
typedef int   FnMcsGraphicBufferUnmap(void* buffer);
typedef void* FnMcsGraphicBufferGetNativeBuffer(void* buffer);
typedef unsigned int FnMcsGraphicBufferGetStride(void* buffer);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
