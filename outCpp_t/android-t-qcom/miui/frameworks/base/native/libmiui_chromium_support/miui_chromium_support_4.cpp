#include "miui_chromium_support.h"

#include <errno.h>
#include <private/hwui/DrawGlInfo.h>
#include <utils/Functor.h>
#include <ui/GraphicBuffer.h>

static McsDrawGLCallback* g_mcs_drawgl_callback = NULL;

using namespace android;

class McsDrawGLFunctor : public Functor {
public:
  McsDrawGLFunctor(void* context) : context_(context) {}
  virtual ~McsDrawGLFunctor() {}

  virtual status_t operator () (int what, void* data) {
    using uirenderer::DrawGlInfo;
    if (!g_mcs_drawgl_callback) {
        return DrawGlInfo::kStatusDone;
    }

    McsDrawGLInfo mcs_info;
    mcs_info.mode = (what == DrawGlInfo::kModeProcess) ?
        McsDrawGLInfo::kModeProcess : McsDrawGLInfo::kModeDraw;

    DrawGlInfo* gl_info = reinterpret_cast<DrawGlInfo*>(data);

    mcs_info.clip_left = gl_info->clipLeft;
    mcs_info.clip_top = gl_info->clipTop;
    mcs_info.clip_right = gl_info->clipRight;
    mcs_info.clip_bottom = gl_info->clipBottom;
    mcs_info.width = gl_info->width;
    mcs_info.height = gl_info->height;
    mcs_info.is_layer = gl_info->isLayer;

    for (unsigned int i=0; i < sizeof(gl_info->transform) / sizeof(float); ++i) {
        mcs_info.transform[i] = gl_info->transform[i];
    }

    mcs_info.status_mask = McsDrawGLInfo::kStatusMaskDone;
    mcs_info.dirty_left = gl_info->dirtyLeft;
    mcs_info.dirty_top = gl_info->dirtyTop;
    mcs_info.dirty_right = gl_info->dirtyRight;
    mcs_info.dirty_bottom = gl_info->dirtyBottom;

    g_mcs_drawgl_callback(reinterpret_cast<long>(context_), &mcs_info);

    // Copy out the outputs.
    gl_info->dirtyLeft = mcs_info.dirty_left;
    gl_info->dirtyTop = mcs_info.dirty_top;
    gl_info->dirtyRight = mcs_info.dirty_right;
    gl_info->dirtyBottom = mcs_info.dirty_bottom;

    status_t res = DrawGlInfo::kStatusDone;
    if (mcs_info.status_mask & McsDrawGLInfo::kStatusMaskDraw)
      res |= DrawGlInfo::kStatusDraw;
    if (mcs_info.status_mask & McsDrawGLInfo::kStatusMaskInvoke)
      res |= DrawGlInfo::kStatusInvoke;

    return res;
  }

  private:
    void* context_;
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
  return new McsDrawGLFunctor(context);
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

  return buffer;
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

