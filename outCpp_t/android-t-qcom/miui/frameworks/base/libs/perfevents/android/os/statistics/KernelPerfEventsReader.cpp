#define LOG_TAG "KernelPerfEventsReader"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/atomic.h>

#include "jni.h"

#include "PerfEventConstants.h"
#include "PerfSuperviser.h"
#include "KernelPerfEventsReader.h"
#include "FastSystemInfoFetcher.h"
#include "NativePerfEventParcel.h"
#include "NativePerfEventParcelCache.h"
#include "PerfEvent.h"
#include "PerfEventDetailsCache.h"
#include "ShortCString.h"
#include "ShortCStringCache.h"

namespace android {
namespace os {
namespace statistics {

#define BUFFERITEM_FIELD_TYPE_U64 0
#define BUFFERITEM_FIELD_TYPE_S64 1
#define BUFFERITEM_FIELD_TYPE_U32 2
#define BUFFERITEM_FIELD_TYPE_S32 3
#define BUFFERITEM_FIELD_TYPE_STR 4
#define BUFFERITEM_FIELD_TYPE_BACKTRACE 5

#define KPERFEVENT_TYPE_UNKNOWN 0
#define KPERFEVENT_TYPE_SAME_PROCESS_SCHED_WAIT 1
#define KPERFEVENT_TYPE_CROSS_PROCESS_SCHED_WAIT 2
#define KPERFEVENT_TYPE_SAME_PROCESS_SCHED_WAKE 3
#define KPERFEVENT_TYPE_CROSS_PROCESS_SCHED_WAKE 4
#define KPERFEVENT_TYPE_MM_SLOWPATH 5

struct PerfEventBufferItem {
  uint32_t type:4;
  uint32_t reserved:4;
  uint32_t item_length:12;
  uint32_t data_length:12;
  char array[] __attribute__((aligned(4)));
};

static inline unsigned int peekBufferItemFieldType(struct PerfEventBufferItem* item, unsigned int pos) {
  return (unsigned int)item->array[pos];
}

static inline unsigned int readUint64(
    struct PerfEventBufferItem* item, unsigned int pos, uint64_t* valuePtr) {
  char* target = (char*)valuePtr;
  unsigned int tail = pos + 1 + sizeof(uint64_t);

  target[0] = item->array[pos + 1];
  target[1] = item->array[pos + 2];
  target[2] = item->array[pos + 3];
  target[3] = item->array[pos + 4];
  target[4] = item->array[pos + 5];
  target[5] = item->array[pos + 6];
  target[6] = item->array[pos + 7];
  target[7] = item->array[pos + 8];

  return tail;
}

static inline unsigned int readInt64(
    struct PerfEventBufferItem* item, unsigned int pos, int64_t* valuePtr) {
  char* target = (char*)valuePtr;
  unsigned int tail = pos + 1 + sizeof(int64_t);

  target[0] = item->array[pos + 1];
  target[1] = item->array[pos + 2];
  target[2] = item->array[pos + 3];
  target[3] = item->array[pos + 4];
  target[4] = item->array[pos + 5];
  target[5] = item->array[pos + 6];
  target[6] = item->array[pos + 7];
  target[7] = item->array[pos + 8];

  return tail;
}

static inline unsigned int readUint32(
    struct PerfEventBufferItem* item, unsigned int pos, uint32_t *valuePtr) {
  char* target = (char*)valuePtr;
  unsigned int tail = pos + 1 + sizeof(uint32_t);

  target[0] = item->array[pos + 1];
  target[1] = item->array[pos + 2];
  target[2] = item->array[pos + 3];
  target[3] = item->array[pos + 4];

  return tail;
}

static inline unsigned int readInt32(
    struct PerfEventBufferItem* item, unsigned int pos, int32_t *valuePtr) {
  char* target = (char*)valuePtr;
  unsigned int tail = pos + 1 + sizeof(int32_t);

  target[0] = item->array[pos + 1];
  target[1] = item->array[pos + 2];
  target[2] = item->array[pos + 3];
  target[3] = item->array[pos + 4];

  return tail;
}

static inline uint32_t peekLengthOfStr(struct PerfEventBufferItem* item, unsigned int pos) {
  uint32_t count;
  char* target = (char*)(&count);

  target[0] = item->array[pos + 1];
  target[1] = item->array[pos + 2];
  target[2] = item->array[pos + 3];
  target[3] = item->array[pos + 4];

  return count;
}

static inline unsigned int readInlineStr(
    struct PerfEventBufferItem* item, unsigned int pos, char** resultStr) {
  uint32_t length = peekLengthOfStr(item, pos);
  unsigned int tail = pos + 1 + sizeof(uint32_t) + length + 1;

  *resultStr = item->array + pos + 1 + sizeof(uint32_t);

  return tail;
}

static inline uint32_t peekDepthOfKernelBacktrace(struct PerfEventBufferItem* item, unsigned int pos) {
  uint32_t depth;
  char* target = (char*)(&depth);

  target[0] = item->array[pos + 1];
  target[1] = item->array[pos + 2];
  target[2] = item->array[pos + 3];
  target[3] = item->array[pos + 4];

  return depth;
}

static inline unsigned int readKernelBacktrace(
    struct PerfEventBufferItem* item, unsigned int pos,
    jlong* kernelBacktraceBuf, uint32_t bufSize, uint32_t& depth) {
  depth = peekDepthOfKernelBacktrace(item, pos);
  uint32_t tail = pos + 1 + sizeof(uint32_t) + depth * sizeof(uint64_t);
  pos += 5;
  if (depth > bufSize) depth = bufSize;
  for (uint32_t i = 0; i < depth; i++) {
    uint64_t pc;
    char* ptr= (char *)&pc;
    ptr[0] = item->array[pos + 0];
    ptr[1] = item->array[pos + 1];
    ptr[2] = item->array[pos + 2];
    ptr[3] = item->array[pos + 3];
    ptr[4] = item->array[pos + 4];
    ptr[5] = item->array[pos + 5];
    ptr[6] = item->array[pos + 6];
    ptr[7] = item->array[pos + 7];
    *(kernelBacktraceBuf + i) = (jlong)pc;
    pos += 8;
  }
  return tail;
}

static inline uint32_t peekLengthOfComplexField(struct PerfEventBufferItem* item, unsigned int pos) {
  uint32_t count;
  char* target = (char*)(&count);

  target[0] = item->array[pos + 1];
  target[1] = item->array[pos + 2];
  target[2] = item->array[pos + 3];
  target[3] = item->array[pos + 4];

  return count;
}

static inline unsigned int skip(
    struct PerfEventBufferItem* item, unsigned int pos) {
  uint32_t length = peekLengthOfComplexField(item, pos);
  unsigned int tail = 1 + sizeof(uint32_t) + length;
  return tail;
}

class KernelPerfEventsBuffer {
public:
  static KernelPerfEventsBuffer* create(uint32_t size);

public:
  KernelPerfEventsBuffer(uint32_t size);
  ~KernelPerfEventsBuffer();
  inline bool isEmpty() {
    return m_readPos >= m_writePos;
  }
  struct PerfEventBufferItem* next();

public:
  void *m_data;
  int m_bufferSize;
  int m_writePos;
  int m_readPos;
};

KernelPerfEventsBuffer* KernelPerfEventsBuffer::create(uint32_t size) {
  KernelPerfEventsBuffer* buffer = new KernelPerfEventsBuffer(size);
  if (buffer == NULL) {
    return NULL;
  }

  if (buffer->m_data == NULL) {
    delete buffer;
    return NULL;
  }

  return buffer;
}

KernelPerfEventsBuffer::KernelPerfEventsBuffer(uint32_t size) {
  m_readPos = 0;
  m_writePos = 0;
  if (posix_memalign(&m_data, 8, size) == 0) {
    m_bufferSize = size;
  } else {
    m_data = NULL;
    m_bufferSize = 0;
  }
}

KernelPerfEventsBuffer::~KernelPerfEventsBuffer() {
  if (m_data != NULL) {
    free(m_data);
  }
}

struct PerfEventBufferItem* KernelPerfEventsBuffer::next() {
  if (m_readPos < m_writePos) {
    struct PerfEventBufferItem* bufferItem =
        (struct PerfEventBufferItem*) ((char *)m_data + m_readPos);
    m_readPos += bufferItem->item_length;
    return bufferItem;
  } else {
    return NULL;
  }
}

} //namespace statistics
} //namespace os
} //namespace android

using namespace android;
using namespace android::os::statistics;

static const char* proc_kernel_perfevents_file = "/dev/proc_kperfevents";
static const char* device_kernel_perfevents_file = "/dev/device_kperfevents";
static const char* lower_threshold_millis_file = "/d/kperfevents/lower_threshold_millis";
static const char* upper_threshold_millis_file = "/d/kperfevents/upper_threshold_millis";
static const char* min_overlap_millis_file = "/d/kperfevents/min_overlap_millis";
static const char* supervision_level_file = "/d/kperfevents/supervision_level";

static const char* const kFilteringPerfEventFactoryPathName = "android/os/statistics/FilteringPerfEventFactory";
static jclass gFilteringPerfEventFactoryClass;
static jmethodID gCreateFilteringPerfEventMethod;

static atomic_int_fast64_t gEventSeq;

static KernelPerfEventsReader *gProcKernelPerfEventsReader = NULL;

static void writeIntToDebugfsNode(const char* filePath, int32_t value) {
  int fd = open(filePath, O_WRONLY);
  if (fd < 0) {
    return;
  }

  char str[32];
  str[0] = '\0';
  int count = snprintf(str, sizeof(str), "%d", value);
  write(fd, str, count + 1);

  close(fd);
}

void KernelPerfEventsReader::start(JNIEnv* env) {
  jclass clazz = FindClassOrDie(env, kFilteringPerfEventFactoryPathName);
  gFilteringPerfEventFactoryClass = MakeGlobalRefOrDie(env, clazz);
  gCreateFilteringPerfEventMethod = GetStaticMethodIDOrDie(env, clazz, "createFilteringPerfEvent",
        "(IIJJJJJJ)Landroid/os/statistics/FilteringPerfEvent;");
  if (PerfSuperviser::isSystemServer() && FastSystemInfoFetcher::hasMiSysInfo()) {
    writeIntToDebugfsNode(lower_threshold_millis_file, PerfSuperviser::getInnerWaitThresholdMs());
    writeIntToDebugfsNode(upper_threshold_millis_file, PerfSuperviser::getsMaxEventThresholdMs());
    writeIntToDebugfsNode(min_overlap_millis_file, PerfSuperviser::getMinOverlappedMs());
    writeIntToDebugfsNode(supervision_level_file, PerfSuperviser::getMode());
  }
  //用户空间事件的seq从int64取值范围的中间开始，内核事件的seq从0开始，
  //这样，对于同一线程相同时间范围内的事件，可以保证内核事件总是被包含在用户空间事件中。
  atomic_init(&gEventSeq, (int64_t)0);
}

KernelPerfEventsReader* KernelPerfEventsReader::openKernelPerfEventsReader(
    const char* filePath, uint32_t bufferSize) {
  if (!FastSystemInfoFetcher::hasMiSysInfo())
    return NULL;

  int fd = open(filePath, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
  if (fd < 0) {
    return NULL;
  }

  KernelPerfEventsBuffer *buffer = KernelPerfEventsBuffer::create(bufferSize);
  if (buffer == NULL) {
      close(fd);
      return NULL;
  }

  KernelPerfEventsReader* reader = new KernelPerfEventsReader();
  if (reader == NULL) {
    close(fd);
    delete buffer;
    return NULL;
  }

  reader->m_fd = fd;
  reader->m_buffer = buffer;
  return reader;
}

KernelPerfEventsReader* KernelPerfEventsReader::openProcKernelPerfEventsReader(uint32_t bufferSize) {
  gProcKernelPerfEventsReader = openKernelPerfEventsReader(proc_kernel_perfevents_file, bufferSize);
  return gProcKernelPerfEventsReader;
}

KernelPerfEventsReader* KernelPerfEventsReader::openDeviceKernelPerfEventsReader(uint32_t bufferSize) {
  return openKernelPerfEventsReader(device_kernel_perfevents_file, bufferSize);
}

KernelPerfEventsReader* KernelPerfEventsReader::getProcKernelPerfEventsReader() {
  return gProcKernelPerfEventsReader;
}

bool KernelPerfEventsReader::isEmpty() {
  return m_buffer != NULL && m_buffer->isEmpty();
}

static jobject parse(JNIEnv* env, struct PerfEventBufferItem* bufferItem) {
  unsigned int pos = 0;
  union {
    uint64_t u64Value;
    int64_t s64Value;
    uint32_t u32Value;
    int32_t s32Value;
    char* strValue;
  } field_value;
  jlong kernelBacktrace[32];
  uint32_t depth;
  bool needProcessName = false;

  PerfEvent perfEvent;

  pos = readUint32(bufferItem, pos, &(field_value.u32Value));
  switch (field_value.u32Value) {
  case KPERFEVENT_TYPE_SAME_PROCESS_SCHED_WAIT:
    perfEvent.mEventType = TYPE_SCHED_WAIT;
    perfEvent.mEventFlags = FLAG_BLOCKED | FLAG_BLOCKED_BY_SAME_PROCESS | FLAG_BLOCKED_BY_ONE_COINCIDED_BLOCKER;
    break;
  case KPERFEVENT_TYPE_CROSS_PROCESS_SCHED_WAIT:
    perfEvent.mEventType = TYPE_SCHED_WAIT;
    perfEvent.mEventFlags = FLAG_BLOCKED | FLAG_BLOCKED_BY_CROSS_PROCESS | FLAG_BLOCKED_BY_ONE_COINCIDED_BLOCKER;
    break;
  case KPERFEVENT_TYPE_SAME_PROCESS_SCHED_WAKE:
    perfEvent.mEventType = TYPE_SCHED_WAKE;
    perfEvent.mEventFlags = FLAG_BLOCKER | FLAG_BLOCKER_TO_SAME_PROCESS;
    needProcessName = true;
    break;
  case KPERFEVENT_TYPE_CROSS_PROCESS_SCHED_WAKE:
    perfEvent.mEventType = TYPE_SCHED_WAKE;
    perfEvent.mEventFlags = FLAG_BLOCKER | FLAG_BLOCKER_TO_CROSS_PROCESS | FLAG_INITIATOR_POSITION_SLAVE;
    needProcessName = true;
    break;
  case KPERFEVENT_TYPE_MM_SLOWPATH:
    perfEvent.mEventType = TYPE_MM_SLOWPATH;
    perfEvent.mEventFlags = 0;
    break;
  default:
    perfEvent.mEventType = TYPE_INVALID_PERF_EVENT;
    perfEvent.mEventFlags = 0;
    break;
  }
  if (perfEvent.mEventType == (int32_t)TYPE_INVALID_PERF_EVENT) {
    return nullptr;
  }

  pos = readUint64(bufferItem, pos, (uint64_t *)&(perfEvent.mBeginUptimeMillis));
  pos = readUint64(bufferItem, pos, (uint64_t *)&(perfEvent.mEndUptimeMillis));
  pos = readInt64(bufferItem, pos, &(perfEvent.mInclusionId));
  pos = readInt64(bufferItem, pos, &(perfEvent.mSynchronizationId));
  perfEvent.mEventSeq = atomic_fetch_add_explicit(&gEventSeq, (int64_t)1, memory_order_relaxed);

  NativePerfEventParcel* nativeParcel = NativePerfEventParcelCache::obtain();
  if (needProcessName) {
    if (!PerfSuperviser::isMiuiDaemon()) {
        nativeParcel->writeProcessName();
    } else {
      nativeParcel->writeShortCString(nullptr);
    }
  }
  while (pos < bufferItem->data_length) {
    unsigned int fieldType = peekBufferItemFieldType(bufferItem, pos);
    switch (fieldType) {
    case BUFFERITEM_FIELD_TYPE_U64:
      pos = readUint64(bufferItem, pos, &(field_value.u64Value));
      nativeParcel->writeInt64(field_value.u64Value);
      break;
    case BUFFERITEM_FIELD_TYPE_S64:
      pos = readInt64(bufferItem, pos, &(field_value.s64Value));
      nativeParcel->writeInt64(field_value.s64Value);
      break;
    case BUFFERITEM_FIELD_TYPE_U32:
      pos = readUint32(bufferItem, pos, &(field_value.u32Value));
      nativeParcel->writeInt32(field_value.u32Value);
      break;
    case BUFFERITEM_FIELD_TYPE_S32:
      pos = readInt32(bufferItem, pos, &(field_value.s32Value));
      nativeParcel->writeInt32(field_value.s32Value);
      break;
    case BUFFERITEM_FIELD_TYPE_STR:
      pos = readInlineStr(bufferItem, pos,  &(field_value.strValue));
      nativeParcel->writeShortCString(field_value.strValue);
      break;
    case BUFFERITEM_FIELD_TYPE_BACKTRACE:
      depth = 0;
      pos = readKernelBacktrace(bufferItem, pos, kernelBacktrace, sizeof(kernelBacktrace), depth);
      //TODO:暂时没有用到，先不处理
      break;
    default://未支持的复杂数据类型，跳过
      pos = skip(bufferItem, pos);
      break;
    }
  }

  PerfEventDetails *details = PerfEventDetailsCache::obtain();
  details->mGlobalDetails = nativeParcel;
  details->mGlobalJavaBackTrace = nullptr;
  perfEvent.mEventFlags |= FLAG_FROM_PERFEVENT_DETAILS;
  return env->CallStaticObjectMethod(gFilteringPerfEventFactoryClass, gCreateFilteringPerfEventMethod,
    (jint)perfEvent.mEventType, (jint)perfEvent.mEventFlags,
    (jlong)perfEvent.mBeginUptimeMillis, (jlong)perfEvent.mEndUptimeMillis,
    (jlong)perfEvent.mInclusionId, (jlong)perfEvent.mSynchronizationId,
    (jlong)perfEvent.mEventSeq,
    (jlong)details);
}

int KernelPerfEventsReader::readPerfEvents(JNIEnv* env, jobjectArray resultBuffer) {
  const uint32_t resultBufferSize = env->GetArrayLength(resultBuffer);
  uint32_t count = 0;
  int ret = 0;
  struct PerfEventBufferItem* bufferItem;

  int tryTimes = 0;
  int sleepMillis = (PerfSuperviser::getInnerWaitThresholdMs()) / 8;
  if (sleepMillis < 2) {
    sleepMillis = 2;
  }

  while (true) {
    bufferItem = m_buffer->next();
    if (bufferItem != NULL) {
      jobject perfEvent = parse(env, bufferItem);
      if (perfEvent != nullptr) {
        env->SetObjectArrayElement(resultBuffer, count, perfEvent);
        env->DeleteLocalRef(perfEvent);
        count++;
      }
      if (count == resultBufferSize) {
        break;
      }
      if (!m_buffer->isEmpty()) {
        continue;
      }
    }
    ret = read(m_fd, m_buffer->m_data, m_buffer->m_bufferSize);
    if (ret == 0) {
      break;
    } else if (ret < 0) {
      if ((errno == EINTR || errno == EBUSY) && tryTimes < 16) {
        usleep(sleepMillis * 1000);
        tryTimes++;
        continue;
      } else {
        break;
      }
    }
    m_buffer->m_readPos = 0;
    m_buffer->m_writePos = ret;
  }

  return count;
}
