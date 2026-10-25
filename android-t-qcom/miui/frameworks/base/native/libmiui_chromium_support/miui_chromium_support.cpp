#include "miui_chromium_support.h"

#include <errno.h>
#include <jni.h>
#include <private/hwui/DrawGlInfo.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <utils/Functor.h>
#include <ui/GraphicBuffer.h>

#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#define COMPILE_ASSERT(expr, err) static const char err[(expr) ? 1 : -1] = "";

static McsDrawGLCallback* g_mcs_drawgl_callback = NULL;

using namespace android;

class McsDrawGLFunctor : public Functor {
public:
  McsDrawGLFunctor(jlong context) : context_(context) {}
  virtual ~McsDrawGLFunctor() {}

  virtual status_t operator () (int what, void* data) {
    using uirenderer::DrawGlInfo;
    if (!g_mcs_drawgl_callback) {
        return DrawGlInfo::kStatusDone;
    }

    McsDrawGLInfo mcs_info;
    // mcs_info.version = kMcsDrawGLInfoVersion;

    switch (what) {
      case DrawGlInfo::kModeDraw: {
        mcs_info.mode = McsDrawGLInfo::kModeDraw;
        DrawGlInfo* gl_info = reinterpret_cast<DrawGlInfo*>(data);

        // Map across the input values.
        mcs_info.clip_left = gl_info->clipLeft;
        mcs_info.clip_top = gl_info->clipTop;
        mcs_info.clip_right = gl_info->clipRight;
        mcs_info.clip_bottom = gl_info->clipBottom;
        mcs_info.width = gl_info->width;
        mcs_info.height = gl_info->height;
        mcs_info.is_layer = gl_info->isLayer;
        COMPILE_ASSERT(NELEM(mcs_info.transform) == NELEM(gl_info->transform),
                       mismatched_transform_matrix_sizes);
        for (int i = 0; i < NELEM(mcs_info.transform); ++i) {
          mcs_info.transform[i] = gl_info->transform[i];
        }
        break;
      }
      case DrawGlInfo::kModeProcess:
        mcs_info.mode = McsDrawGLInfo::kModeProcess;
        break;
      case DrawGlInfo::kModeProcessNoContext:
        mcs_info.mode = McsDrawGLInfo::kModeProcessNoContext;
        break;
      case DrawGlInfo::kModeSync:
        mcs_info.mode = McsDrawGLInfo::kModeSync;
        break;
      default:
        return DrawGlInfo::kStatusDone;
    }

    // Invoke the DrawGL method.
    // TODO(shouqun); The third parameter is not used.
    g_mcs_drawgl_callback(context_, &mcs_info/*, NULL*/);
    return DrawGlInfo::kStatusDone;
  }

  private:
    intptr_t context_;
};

class GraphicBufferImpl {
public:
  ~GraphicBufferImpl();
  status_t Map(McsMapMode mode, void** vaddr);
  status_t Unmap();
  status_t InitCheck() const;
  void* GetNativeBuffer() const;
  uint32_t GetStride() const;
  GraphicBufferImpl(uint32_t w, uint32_t h);

private:
  sp<android::GraphicBuffer> mBuffer;
};

GraphicBufferImpl::GraphicBufferImpl(uint32_t w, uint32_t h)
  : mBuffer(new android::GraphicBuffer(w, h, PIXEL_FORMAT_RGBA_8888,
       android::GraphicBuffer::USAGE_HW_TEXTURE |
       android::GraphicBuffer::USAGE_SW_READ_OFTEN |
       android::GraphicBuffer::USAGE_SW_WRITE_OFTEN)) {
}

GraphicBufferImpl::~GraphicBufferImpl() {
}

status_t GraphicBufferImpl::Map(McsMapMode mode, void** vaddr) {
  int usage = 0;
  switch (mode) {
    case MCS_MAP_READ_ONLY:
      usage = android::GraphicBuffer::USAGE_SW_READ_OFTEN;
      break;
    case MCS_MAP_WRITE_ONLY:
      usage = android::GraphicBuffer::USAGE_SW_WRITE_OFTEN;
      break;
    case MCS_MAP_READ_WRITE:
      usage = android::GraphicBuffer::USAGE_SW_READ_OFTEN |
          android::GraphicBuffer::USAGE_SW_WRITE_OFTEN;
      break;
    default:
      return INVALID_OPERATION;
  }
  return mBuffer->lock(usage, vaddr);
}

status_t GraphicBufferImpl::Unmap() {
  return mBuffer->unlock();
}

status_t GraphicBufferImpl::InitCheck() const {
  return mBuffer->initCheck();
}

void* GraphicBufferImpl::GetNativeBuffer() const {
  return mBuffer->getNativeBuffer();
}

uint32_t GraphicBufferImpl::GetStride() const {
  static const int kBytesPerPixel = 4;
  return mBuffer->getStride() * kBytesPerPixel;
}


//////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

void* McsGLFunctorCreate(void* context) {
  return new McsDrawGLFunctor(reinterpret_cast<jlong>(context));
}

void  McsGLFunctorDestroy(void* functor) {
  delete reinterpret_cast<McsDrawGLFunctor*>(functor);
}

void McsGLFunctorSetCallback(McsDrawGLCallback callback) {
  g_mcs_drawgl_callback = callback;
}

void* McsGraphicBufferCreate(int w, int h) {
  GraphicBufferImpl* buffer = new GraphicBufferImpl(
      static_cast<uint32_t>(w),
      static_cast<uint32_t>(h));

  if (buffer->InitCheck() != NO_ERROR) {
    delete buffer;
    return 0;
  }

  return reinterpret_cast<void*>(buffer);
}

void  McsGraphicBufferRelease(void* buffer) {
  GraphicBufferImpl* impl = reinterpret_cast<GraphicBufferImpl*>(buffer);
  delete reinterpret_cast<GraphicBufferImpl*>(impl);
}

int   McsGraphicBufferMap(void* buffer, McsMapMode mode, void** vaddr) {
  GraphicBufferImpl* impl = reinterpret_cast<GraphicBufferImpl*>(buffer);
  return impl->Map(mode, vaddr);
}

int   McsGraphicBufferUnmap(void* buffer) {
  GraphicBufferImpl* impl = reinterpret_cast<GraphicBufferImpl*>(buffer);
  return impl->Unmap();
}

void* McsGraphicBufferGetNativeBuffer(void* buffer) {
  GraphicBufferImpl* impl = reinterpret_cast<GraphicBufferImpl*>(buffer);
  return impl->GetNativeBuffer();
}

unsigned int McsGraphicBufferGetStride(void* buffer) {
  GraphicBufferImpl* impl = reinterpret_cast<GraphicBufferImpl*>(buffer);
  return impl->GetStride();
}

#ifdef __cplusplus
} //extern "C"
#endif

