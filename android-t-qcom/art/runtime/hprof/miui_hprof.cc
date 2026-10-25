#include "hprof.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#include <set>

#include <android-base/logging.h>
#include <android-base/stringprintf.h>

#include "art_field-inl.h"
#include "art_method-inl.h"
#include "base/array_ref.h"
#include "base/file_utils.h"
#include "base/locks.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "base/os.h"
#include "base/safe_map.h"
#include "base/time_utils.h"
#include "base/unix_file/fd_file.h"
#include "class_linker.h"
#include "class_root-inl.h"
#include "common_throws.h"
#include "debugger.h"
#include "dex/dex_file-inl.h"
#include "dex/primitive.h"
#include "gc/scoped_gc_critical_section.h"
#include "gc/heap-visit-objects-inl.h"
#include "gc/heap.h"
#include "gc/scoped_gc_critical_section.h"
#include "gc/space/space.h"
#include "gc_root.h"
#include "mirror/class-inl.h"
#include "mirror/class.h"
#include "mirror/object-refvisitor-inl.h"
#include "runtime_globals.h"
#include "scoped_thread_state_change-inl.h"
#include "thread-current-inl.h"
#include "thread_list.h"

namespace art {

namespace hprof {

static constexpr size_t kMaxObjectsPerSegment = 128;
static constexpr size_t kMaxBytesPerSegment = 4096;

enum HprofTag {
  HPROF_TAG_STRING = 0x01,
  HPROF_TAG_LOAD_CLASS = 0x02,
  HPROF_TAG_HEAP_DUMP_SEGMENT = 0x1C,
  HPROF_TAG_HEAP_DUMP_END = 0x2C,
};

// Values for the first byte of HEAP_DUMP and HEAP_DUMP_SEGMENT records:
enum HprofHeapTag {
  // Traditional.
  HPROF_ROOT_UNKNOWN = 0xFF,
  HPROF_ROOT_JNI_GLOBAL = 0x01,
  HPROF_ROOT_JNI_LOCAL = 0x02,
  HPROF_ROOT_JAVA_FRAME = 0x03,
  HPROF_ROOT_NATIVE_STACK = 0x04,
  HPROF_ROOT_STICKY_CLASS = 0x05,
  HPROF_ROOT_THREAD_BLOCK = 0x06,
  HPROF_ROOT_MONITOR_USED = 0x07,
  HPROF_ROOT_THREAD_OBJECT = 0x08,
  HPROF_CLASS_DUMP = 0x20,
  HPROF_INSTANCE_DUMP = 0x21,
  HPROF_OBJECT_ARRAY_DUMP = 0x22,
  HPROF_PRIMITIVE_ARRAY_DUMP = 0x23,

  // Android.
  HPROF_HEAP_DUMP_INFO = 0xfe,
  HPROF_ROOT_INTERNED_STRING = 0x89,
  HPROF_ROOT_FINALIZING = 0x8a,  // Obsolete.
  HPROF_ROOT_DEBUGGER = 0x8b,
  HPROF_ROOT_REFERENCE_CLEANUP = 0x8c,  // Obsolete.
  HPROF_ROOT_VM_INTERNAL = 0x8d,
  HPROF_ROOT_JNI_MONITOR = 0x8e,
  HPROF_UNREACHABLE = 0x90,  // Obsolete.
  HPROF_PRIMITIVE_ARRAY_NODATA_DUMP = 0xc3,  // Obsolete.
};

using HprofStringId = uint32_t;
using HprofClassObjectId = uint32_t;

class StripEndianOutput {
 public:
  StripEndianOutput() : length_(0), sum_length_(0), max_length_(0), started_(false) {}
  virtual ~StripEndianOutput() {}

  void StartNewRecord(uint8_t tag) {
    if (length_ > 0) {
      EndRecord();
    }
    DCHECK_EQ(length_, 0U);
    AddU1(tag);
    AddU4(0xdeaddead);  // Length, replaced on flush.
    started_ = true;
  }

  void EndRecord() {
    // Replace length in header.
    if (started_) {
      UpdateU4(sizeof(uint8_t), length_ - sizeof(uint8_t) - sizeof(uint32_t));
    }

    HandleEndRecord();

    sum_length_ += length_;
    max_length_ = std::max(max_length_, length_);
    length_ = 0;
    started_ = false;
  }

  void AddU1(uint8_t value) {
    AddU1List(&value, 1);
  }
  void AddU2(uint16_t value) {
    AddU2List(&value, 1);
  }
  void AddU4(uint32_t value) {
    AddU4List(&value, 1);
  }
  void AddU8(uint64_t value) {
    AddU8List(&value, 1);
  }

  void AddObjectId(const mirror::Object* value) {
    AddU4(PointerToLowMemUInt32(value));
  }

  void AddClassId(HprofClassObjectId value) {
    AddU4(value);
  }

  void AddStringId(HprofStringId value) {
    AddU4(value);
  }

  void AddU1List(const uint8_t* values, size_t count) {
    HandleU1List(values, count);
    length_ += count;
  }
  void AddU2List(const uint16_t* values, size_t count) {
    HandleU2List(values, count);
    length_ += count * sizeof(uint16_t);
  }
  void AddU4List(const uint32_t* values, size_t count) {
    HandleU4List(values, count);
    length_ += count * sizeof(uint32_t);
  }
  void AddU8List(const uint64_t* values, size_t count) {
    HandleU8List(values, count);
    length_ += count * sizeof(uint64_t);
  }

  void AddIdList(mirror::ObjectArray<mirror::Object>* values)
      REQUIRES_SHARED(Locks::mutator_lock_) {
    const int32_t length = values->GetLength();
    for (int32_t i = 0; i < length; ++i) {
      AddObjectId(values->GetWithoutChecks(i).Ptr());
    }
  }

  void AddUtf8String(const char* str) {
    // The terminating NUL character is NOT written.
    AddU1List((const uint8_t*)str, strlen(str));
  }

  size_t Length() const {
    return length_;
  }

  size_t SumLength() const {
    return sum_length_;
  }

  size_t MaxLength() const {
    return max_length_;
  }

  virtual void UpdateU2(size_t offset, uint16_t new_value ATTRIBUTE_UNUSED) {
    DCHECK_LE(offset, length_ - 2);
  }
  virtual void UpdateU4(size_t offset, uint32_t new_value ATTRIBUTE_UNUSED) {
    DCHECK_LE(offset, length_ - 4);
  }

 protected:
  virtual void HandleU1List(const uint8_t* values ATTRIBUTE_UNUSED,
                            size_t count ATTRIBUTE_UNUSED) {
  }
  virtual void HandleU2List(const uint16_t* values ATTRIBUTE_UNUSED,
                            size_t count ATTRIBUTE_UNUSED) {
  }
  virtual void HandleU4List(const uint32_t* values ATTRIBUTE_UNUSED,
                            size_t count ATTRIBUTE_UNUSED) {
  }
  virtual void HandleU8List(const uint64_t* values ATTRIBUTE_UNUSED,
                            size_t count ATTRIBUTE_UNUSED) {
  }
  virtual void HandleEndRecord() {
  }

  size_t length_;      // Current record size.
  size_t sum_length_;  // Size of all data.
  size_t max_length_;  // Maximum seen length.
  bool started_;       // Was StartRecord called?
};

// This keeps things buffered until flushed.
class StripEndianOutputBuffered : public StripEndianOutput {
 public:
  explicit StripEndianOutputBuffered(size_t reserve_size) {
    buffer_.reserve(reserve_size);
  }
  virtual ~StripEndianOutputBuffered() {}

  void UpdateU2(size_t offset, uint16_t new_value) override {
    DCHECK_LE(offset, length_ - 2);
    buffer_[offset + 0] = static_cast<uint8_t>((new_value >> 8)  & 0xFF);
    buffer_[offset + 1] = static_cast<uint8_t>((new_value >> 0)  & 0xFF);
  }

  void UpdateU4(size_t offset, uint32_t new_value) override {
    DCHECK_LE(offset, length_ - 4);
    buffer_[offset + 0] = static_cast<uint8_t>((new_value >> 24) & 0xFF);
    buffer_[offset + 1] = static_cast<uint8_t>((new_value >> 16) & 0xFF);
    buffer_[offset + 2] = static_cast<uint8_t>((new_value >> 8)  & 0xFF);
    buffer_[offset + 3] = static_cast<uint8_t>((new_value >> 0)  & 0xFF);
  }

 protected:
  void HandleU1List(const uint8_t* values, size_t count) override {
    DCHECK_EQ(length_, buffer_.size());
    buffer_.insert(buffer_.end(), values, values + count);
  }

  void HandleU2List(const uint16_t* values, size_t count) override {
    DCHECK_EQ(length_, buffer_.size());
    for (size_t i = 0; i < count; ++i) {
      uint16_t value = *values;
      buffer_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
      buffer_.push_back(static_cast<uint8_t>((value >> 0) & 0xFF));
      values++;
    }
  }

  void HandleU4List(const uint32_t* values, size_t count) override {
    DCHECK_EQ(length_, buffer_.size());
    for (size_t i = 0; i < count; ++i) {
      uint32_t value = *values;
      buffer_.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
      buffer_.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
      buffer_.push_back(static_cast<uint8_t>((value >> 8)  & 0xFF));
      buffer_.push_back(static_cast<uint8_t>((value >> 0)  & 0xFF));
      values++;
    }
  }

  void HandleU8List(const uint64_t* values, size_t count) override {
    DCHECK_EQ(length_, buffer_.size());
    for (size_t i = 0; i < count; ++i) {
      uint64_t value = *values;
      buffer_.push_back(static_cast<uint8_t>((value >> 56) & 0xFF));
      buffer_.push_back(static_cast<uint8_t>((value >> 48) & 0xFF));
      buffer_.push_back(static_cast<uint8_t>((value >> 40) & 0xFF));
      buffer_.push_back(static_cast<uint8_t>((value >> 32) & 0xFF));
      buffer_.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
      buffer_.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
      buffer_.push_back(static_cast<uint8_t>((value >> 8)  & 0xFF));
      buffer_.push_back(static_cast<uint8_t>((value >> 0)  & 0xFF));
      values++;
    }
  }

  void HandleEndRecord() override {
    DCHECK_EQ(buffer_.size(), length_);
    HandleFlush(buffer_.data(), length_);
    buffer_.clear();
  }

  virtual void HandleFlush(const uint8_t* buffer ATTRIBUTE_UNUSED, size_t length ATTRIBUTE_UNUSED) {
  }

  std::vector<uint8_t> buffer_;
};

class StripFileEndianOutput final : public StripEndianOutputBuffered {
 public:
  StripFileEndianOutput(File* fp, size_t reserved_size)
      : StripEndianOutputBuffered(reserved_size), fp_(fp), errors_(false) {
    DCHECK(fp != nullptr);
  }
  ~StripFileEndianOutput() {
  }

  bool Errors() {
    return errors_;
  }

 protected:
  void HandleFlush(const uint8_t* buffer, size_t length) override {
    if (!errors_) {
      errors_ = !fp_->WriteFully(buffer, length);
    }
  }

 private:
  File* fp_;
  bool errors_;
};

#define __ output_->

class StripHprof : public SingleRootVisitor {
 public:
  StripHprof(const char* output_filename)
      : filename_(output_filename) {
    LOG(INFO) << "hprof: heap dump \"" << filename_ << "\" starting...";
  }

  void Dump()
    REQUIRES(Locks::mutator_lock_)
    REQUIRES(!Locks::heap_bitmap_lock_, !Locks::alloc_tracker_lock_) {

    // First pass to measure the size of the dump.
    size_t overall_size;
    size_t max_length;
    {
      StripEndianOutput count_output;
      output_ = &count_output;
      ProcessHeap(false);
      overall_size = count_output.SumLength();
      max_length = count_output.MaxLength();
      output_ = nullptr;
    }

    visited_objects_.clear();
    bool okay = DumpToFile(overall_size, max_length);

    if (okay) {
      const uint64_t duration = NanoTime() - start_ns_;
      LOG(INFO) << "hprof: heap dump completed (" << PrettySize(RoundUp(overall_size, KB))
                << ") in " << PrettyDuration(duration)
                << " objects " << total_objects_;
    }
  }

 private:
  void DumpHeapObject(mirror::Object* obj)
      REQUIRES_SHARED(Locks::mutator_lock_);

  void DumpHeapClass(mirror::Class* klass)
      REQUIRES_SHARED(Locks::mutator_lock_);

  void DumpHeapArray(mirror::Array* obj, mirror::Class* klass)
      REQUIRES_SHARED(Locks::mutator_lock_);

  void DumpFakeObjectArray(mirror::Object* obj, const std::set<mirror::Object*>& elements)
      REQUIRES_SHARED(Locks::mutator_lock_);

  void DumpHeapInstanceObject(mirror::Object* obj,
                              mirror::Class* klass,
                              const std::set<mirror::Object*>& fake_roots)
      REQUIRES_SHARED(Locks::mutator_lock_);

  bool IsReferenceField(ArtField* field) REQUIRES_SHARED(Locks::mutator_lock_);
  bool IsSpecialReferenceField(ArtField* field) REQUIRES_SHARED(Locks::mutator_lock_);
  bool IsReferenceClass(mirror::Class* klass) REQUIRES_SHARED(Locks::mutator_lock_);
  bool IsReferenceClass(const char* desc) REQUIRES_SHARED(Locks::mutator_lock_);
  bool AddRuntimeInternalObjectsField(mirror::Class* klass) REQUIRES_SHARED(Locks::mutator_lock_);

  void ProcessHeap(bool header_first)
      REQUIRES(Locks::mutator_lock_) {
    // Reset current heap and object count.
    objects_in_segment_ = 0;

    if (header_first) {
      ProcessHeader(true);
      ProcessBody();
    } else {
      ProcessBody();
      ProcessHeader(false);
    }
  }

  void ProcessBody() REQUIRES(Locks::mutator_lock_) {
    Runtime* const runtime = Runtime::Current();
    // Walk the roots and the heap.
    output_->StartNewRecord(HPROF_TAG_HEAP_DUMP_SEGMENT);

    simple_roots_.clear();
    runtime->VisitRoots(this);
    runtime->VisitImageRoots(this);
    auto dump_object = [this](mirror::Object* obj) REQUIRES_SHARED(Locks::mutator_lock_) {
      DCHECK(obj != nullptr);
      DumpHeapObject(obj);
    };
    runtime->GetHeap()->VisitObjectsPaused(dump_object);
    output_->StartNewRecord(HPROF_TAG_HEAP_DUMP_END);
    output_->EndRecord();
  }

  void ProcessHeader(bool string_first) REQUIRES(Locks::mutator_lock_) {
    // Write the header.
    WriteFixedHeader();
    // Write the string and class tables, and any stack traces, to the header.
    // (jhat requires that these appear before any of the data in the body that refers to them.)
    // jhat also requires the string table appear before class table and stack traces.
    // However, WriteStackTraces() can modify the string table, so it's necessary to call
    // WriteStringTable() last in the first pass, to compute the correct length of the output.
    if (string_first) {
      WriteStringTable();
    }
    WriteClassTable();
    if (!string_first) {
      WriteStringTable();
    }
    output_->EndRecord();
  }

  void WriteClassTable() REQUIRES_SHARED(Locks::mutator_lock_) {
    for (const auto& p : classes_) {
      mirror::Class* c = p.first;
      CHECK(c != nullptr);
      output_->StartNewRecord(HPROF_TAG_LOAD_CLASS);

      __ AddObjectId(c);
      __ AddStringId(p.second);
    }
  }

  void WriteStringTable() {
    for (const auto& p : strings_) {
      const std::string& string = p.first;
      const HprofStringId id = p.second;

      output_->StartNewRecord(HPROF_TAG_STRING);

      __ AddU4(id);
      __ AddUtf8String(string.c_str());
    }
  }

  void StartNewHeapDumpSegment() {
    // This flushes the old segment and starts a new one.
    output_->StartNewRecord(HPROF_TAG_HEAP_DUMP_SEGMENT);
    objects_in_segment_ = 0;
  }

  void CheckHeapSegmentConstraints() {
    if (objects_in_segment_ >= kMaxObjectsPerSegment || output_->Length() >= kMaxBytesPerSegment) {
      StartNewHeapDumpSegment();
    }
  }

  void VisitRoot(mirror::Object* obj, const RootInfo& root_info)
      override REQUIRES_SHARED(Locks::mutator_lock_);
  void MarkRootObject(const mirror::Object* obj, HprofHeapTag heap_tag, uint32_t thread_serial);

  HprofClassObjectId LookupClassId(mirror::Class* c) REQUIRES_SHARED(Locks::mutator_lock_) {
    if (c != nullptr) {
      auto it = classes_.find(c);
      if (it == classes_.end()) {
        classes_.Put(c, LookupClassNameId(c));
      }
    }
    return PointerToLowMemUInt32(c);
  }

  HprofStringId LookupStringId(mirror::String* string) REQUIRES_SHARED(Locks::mutator_lock_) {
    return LookupStringId(string->ToModifiedUtf8());
  }

  HprofStringId LookupStringId(const char* string) {
    return LookupStringId(std::string(string));
  }

  HprofStringId LookupStringId(const std::string& string) {
    auto it = strings_.find(string);
    if (it != strings_.end()) {
      return it->second;
    }
    HprofStringId id = next_string_id_++;
    strings_.Put(string, id);
    return id;
  }

  HprofStringId LookupClassNameId(mirror::Class* c) REQUIRES_SHARED(Locks::mutator_lock_) {
    return LookupStringId(c->PrettyDescriptor());
  }

  void WriteFixedHeader() {
    // Write the file header.
    // U1: NUL-terminated magic string.
    const char magic[] = "JAVA PROFILE 1.0.3";
    __ AddU1List(reinterpret_cast<const uint8_t*>(magic), sizeof(magic));

    // U4: size of identifiers.  We're using addresses as IDs and our heap references are stored
    // as uint32_t.
    // Note of warning: hprof-conv hard-codes the size of identifiers to 4.
    static_assert(sizeof(mirror::HeapReference<mirror::Object>) == sizeof(uint32_t),
                  "Unexpected HeapReference size");
    __ AddU4(sizeof(uint32_t));

    // MIUI
    __ AddU4(1296651593);
    __ AddU4(strings_.size());
    __ AddU4(classes_.size());
    __ AddU4(class_count_);
    __ AddU4(class_length_);
    __ AddU4(instance_count_);
    __ AddU4(instance_length_);
    __ AddU4(object_array_count_);
    __ AddU4(object_array_length_);
  }

  bool DumpToFile(size_t overall_size, size_t max_length)
      REQUIRES(Locks::mutator_lock_) {
    int out_fd = open(filename_.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (out_fd < 0) {
      ThrowRuntimeException("Couldn't dump heap; open(\"%s\") failed: %s", filename_.c_str(),
                            strerror(errno));
      return false;
    }

    std::unique_ptr<File> file(new File(out_fd, filename_, true));
    bool okay;
    {
      StripFileEndianOutput file_output(file.get(), max_length);
      output_ = &file_output;
      ProcessHeap(true);
      okay = !file_output.Errors();

      if (okay) {
        // Check for expected size. Output is expected to be less-or-equal than first phase, see
        // b/23521263.
        DCHECK_LE(file_output.SumLength(), overall_size);
      }
      output_ = nullptr;
    }

    if (okay) {
      okay = file->FlushCloseOrErase() == 0;
    } else {
      file->Erase();
    }
    if (!okay) {
      std::string msg(android::base::StringPrintf("Couldn't dump heap; writing \"%s\" failed: %s",
                                                  filename_.c_str(),
                                                  strerror(errno)));
      ThrowRuntimeException("%s", msg.c_str());
      LOG(ERROR) << msg;
    }

    return okay;
  }

  std::string filename_;

  uint64_t start_ns_ = NanoTime();

  StripEndianOutput* output_ = nullptr;

  size_t objects_in_segment_ = 0;
  size_t total_objects_ = 0u;

  size_t class_count_ = 0u;
  size_t class_length_ = 0u;
  size_t instance_count_ = 0u;
  size_t instance_length_ = 0u;
  size_t object_array_count_ = 0u;
  size_t object_array_length_ = 0u;

  HprofStringId next_string_id_ = 0x400000;
  SafeMap<std::string, HprofStringId> strings_;
  SafeMap<mirror::Class*, HprofStringId> classes_;

  // Set used to keep track of what simple root records we have already
  // emitted, to avoid emitting duplicate entries. The simple root records are
  // those that contain no other information than the root type and the object
  // id. A pair of root type and object id is packed into a uint64_t, with
  // the root type in the upper 32 bits and the object id in the lower 32
  // bits.
  std::unordered_set<uint64_t> simple_roots_;

  // To make sure we don't dump the same object multiple times. b/34967844
  std::unordered_set<mirror::Object*> visited_objects_;

  friend class GcRootVisitor;
  DISALLOW_COPY_AND_ASSIGN(StripHprof);
};

bool StripHprof::IsReferenceField(ArtField* field) {
  if (IsSpecialReferenceField(field)) {
    return true;
  }
  const char* desc = field->GetTypeDescriptor();
  if (desc[0] == '[' || desc[0] == 'L') {
    if (desc[0] == 'L' && strcmp(field->GetName(), "shadow$_klass_") == 0) {
      return false;
    } else {
      return IsReferenceClass(desc);
    }
  }
  return false;
}

bool StripHprof::IsSpecialReferenceField(ArtField* field) {
  const char* desc = field->GetTypeDescriptor();
  if (desc[0] == 'Z') {
    // Activity
    if (strcmp(field->GetName(), "mDestroyed") == 0) {
      return true;
    }
    // ViewRootImpl
    if (strcmp(field->GetName(), "mRemoved") == 0) {
      return true;
    }
    // MessageQueue
    if (strcmp(field->GetName(), "mQuitting") == 0) {
      return true;
    }
  } else if (desc[0] == 'I') {
    // View
    if (strcmp(field->GetName(), "mWindowAttachCount") == 0) {
      return true;
    }
    // ArrayList
    if (strcmp(field->GetName(), "size") == 0) {
      return true;
    }
    // Enum
    if (strcmp(field->GetName(), "ordinal") == 0) {
      return true;
    }
  }

  return false;
}

bool StripHprof::IsReferenceClass(mirror::Class* c) {
  CHECK(c != nullptr);
  if (c->IsPrimitive() || c->IsStringClass()) {
    return false;
  }
  std::string temp;
  return IsReferenceClass(c->GetDescriptor(&temp));
}

bool StripHprof::IsReferenceClass(const char* desc) {
  if (desc[0] == '[' || desc[0] == 'L') {
    std::string klass(desc);
    int i = 0, l = strlen(desc);
    while (desc[i] == '[') {
      ++i;
    }
    if (i + 1 == l) {
      // primitive type
      return false;
    } else if (i + 18 == l && klass.compare(i, 18, "Ljava/lang/String;") == 0) {
      // string
      return false;
    }
  }
  return true;
}

bool StripHprof::AddRuntimeInternalObjectsField(mirror::Class* klass) {
  if (klass->IsDexCacheClass()) {
    return true;
  }
  // IsClassLoaderClass is true for subclasses of classloader but we only want to add the fake
  // field to the java.lang.ClassLoader class.
  if (klass->IsClassLoaderClass() && klass->GetSuperClass()->IsObjectClass()) {
    return true;
  }
  return false;
}

void StripHprof::DumpHeapObject(mirror::Object* obj) {
  // Ignore classes that are retired.
  if (obj->IsClass() && obj->AsClass()->IsRetired()) {
    return;
  }

  DCHECK(visited_objects_.insert(obj).second)
      << "Already visited " << obj << "(" << obj->PrettyTypeOf() << ")";

  ++total_objects_;

  class RootCollector {
   public:
    RootCollector() {}

    void operator()(mirror::Object*, MemberOffset, bool) const {}

    // Note that these don't have read barriers. Its OK however since the GC is guaranteed to not be
    // running during the hprof dumping process.
    void VisitRootIfNonNull(mirror::CompressedReference<mirror::Object>* root) const
        REQUIRES_SHARED(Locks::mutator_lock_) {
      if (!root->IsNull()) {
        VisitRoot(root);
      }
    }

    void VisitRoot(mirror::CompressedReference<mirror::Object>* root) const
        REQUIRES_SHARED(Locks::mutator_lock_) {
      roots_.insert(root->AsMirrorPtr());
    }

    const std::set<mirror::Object*>& GetRoots() const {
      return roots_;
    }

   private:
    // These roots are actually live from the object. Avoid marking them as roots in hprof to make
    // it easier to debug class unloading.
    mutable std::set<mirror::Object*> roots_;
  };

  RootCollector visitor;
  // Collect all native roots.
  if (!obj->IsClass()) {
    obj->VisitReferences(visitor, VoidFunctor());
  }

  CheckHeapSegmentConstraints();

  mirror::Class* c = obj->GetClass();
  if (c == nullptr) {
    // This object will bother HprofReader, because it has a null
    // class, so just don't dump it. It could be
    // gDvm.unlinkedJavaLangClass or it could be an object just
    // allocated which hasn't been initialized yet.
  } else {
    if (obj->IsClass()) {
      DumpHeapClass(obj->AsClass().Ptr());
    } else if (c->IsArrayClass()) {
      DumpHeapArray(obj->AsArray().Ptr(), c);
    } else {
      DumpHeapInstanceObject(obj, c, visitor.GetRoots());
    }
  }
}

void StripHprof::DumpHeapClass(mirror::Class* klass) {
  if (!klass->IsResolved()) {
    // Class is allocated but not yet resolved: we cannot access its fields or super class.
    return;
  }
  if (!IsReferenceClass(klass)) {
    return;
  }

  __ AddU1(HPROF_CLASS_DUMP);
  __ AddClassId(LookupClassId(klass));
  __ AddClassId(LookupClassId(klass->GetSuperClass().Ptr()));
  // Instance size.
  if (klass->IsClassClass()) {
    // As mentioned above, we will emit instance fields as synthetic static fields. So the
    // base object is "empty."
    __ AddU4(0);
  } else if (klass->IsArrayClass() || klass->IsPrimitive()) {
    __ AddU4(0);
  } else {
    __ AddU4(klass->GetObjectSize());  // instance size
  }

  // copy static fields and instance fields
  size_t bytes_copy_start = output_->Length();

  // Static fields
  mirror::Class* class_class = klass->GetClass();

  DCHECK(class_class->GetSuperClass()->IsObjectClass());
  const size_t static_fields_reported = class_class->NumInstanceFields()
                                        + class_class->GetSuperClass()->NumInstanceFields()
                                        + klass->NumStaticFields();

  size_t number_of_fields = 0;
  size_t update_number_offset = output_->Length();
  __ AddU2(dchecked_integral_cast<uint16_t>(static_fields_reported));

  // Helper lambda to emit the given static field. The second argument name_fn will be called to
  // generate the name to emit. This can be used to emit something else than the field's actual
  // name.
  auto static_field_writer = [&](ArtField& field, auto name_fn)
      REQUIRES_SHARED(Locks::mutator_lock_) {
    if (IsReferenceField(&field)) {
      __ AddStringId(LookupStringId(name_fn(field)));
      __ AddU4(field.Get32(klass));
      ++number_of_fields;
    }
  };

  {
    auto class_static_field_name_fn = [](ArtField& field) REQUIRES_SHARED(Locks::mutator_lock_) {
      return field.GetName();
    };
    for (ArtField& class_static_field : klass->GetSFields()) {
      static_field_writer(class_static_field, class_static_field_name_fn);
    }
  }

  // Update number of static fields
  __ UpdateU2(update_number_offset, number_of_fields);

  // Instance fields for this class (no superclass fields)
  update_number_offset = output_->Length();
  int iFieldCount = klass->NumInstanceFields();
  // add_internal_runtime_objects is only for classes that may retain objects live through means
  // other than fields. It is never the case for strings.
  const bool add_internal_runtime_objects = AddRuntimeInternalObjectsField(klass);
  if (add_internal_runtime_objects) {
    number_of_fields = 1;
    __ AddU2((uint16_t)iFieldCount + 1);
  } else {
    number_of_fields = 0;
    __ AddU2((uint16_t)iFieldCount);
  }
  for (int i = 0; i < iFieldCount; ++i) {
    ArtField* f = klass->GetInstanceField(i);
    if (IsReferenceField(f)) {
      __ AddStringId(LookupStringId(f->GetName()));
      ++number_of_fields;
    }
  }
  // Add native value character array for strings / byte array for compressed strings.
  if (add_internal_runtime_objects) {
    __ AddStringId(LookupStringId("runtimeInternalObjects"));
  }

  // Update number of instance fields (not including super class's)
  __ UpdateU2(update_number_offset, number_of_fields);

  class_length_ += output_->Length() - bytes_copy_start;

  ++objects_in_segment_;
  ++class_count_;
}

void StripHprof::DumpFakeObjectArray(mirror::Object* obj, const std::set<mirror::Object*>& elements) {
  __ AddU1(HPROF_OBJECT_ARRAY_DUMP);
  __ AddObjectId(obj);
  __ AddU4(elements.size());
  __ AddClassId(LookupClassId(GetClassRoot<mirror::ObjectArray<mirror::Object>>().Ptr()));

  // copy ids list
  size_t bytes_copy_start = output_->Length();
  for (mirror::Object* e : elements) {
    __ AddObjectId(e);
  }
  object_array_length_ += output_->Length() - bytes_copy_start;

  ++objects_in_segment_;
  ++object_array_count_;
}

void StripHprof::DumpHeapArray(mirror::Array* obj, mirror::Class* klass) {
  uint32_t length = obj->GetLength();

  if (obj->IsObjectArray()) {
    if (!IsReferenceClass(klass)) {
      return;
    }

    // obj is an object array.
    __ AddU1(HPROF_OBJECT_ARRAY_DUMP);

    __ AddObjectId(obj);
    __ AddU4(length);
    __ AddClassId(LookupClassId(klass));

    // copy ids list
    size_t bytes_copy_start = output_->Length();

    // Dump the elements, which are always objects or null.
    __ AddIdList(obj->AsObjectArray<mirror::Object>().Ptr());

    object_array_length_ += output_->Length() - bytes_copy_start;

    ++objects_in_segment_;
    ++object_array_count_;
  }
}

void StripHprof::DumpHeapInstanceObject(mirror::Object* obj,
                                   mirror::Class* klass,
                                   const std::set<mirror::Object*>& fake_roots) {
  if (!IsReferenceClass(klass)) {
    return;
  }
  // obj is an instance object.
  __ AddU1(HPROF_INSTANCE_DUMP);
  __ AddObjectId(obj);
  __ AddClassId(LookupClassId(klass));

  // Reserve some space for the length of the instance data, which we won't
  // know until we're done writing it.
  size_t size_patch_offset = output_->Length();
  __ AddU4(0x77777777);

  // copy instance data
  size_t bytes_copy_start = output_->Length();

  // What we will use for the string value if the object is a string.
  mirror::Object* fake_object_array = nullptr;

  // Write the instance data;  fields for this class, followed by super class fields, and so on.
  do {
    const size_t instance_fields = klass->NumInstanceFields();
    for (size_t i = 0; i < instance_fields; ++i) {
      ArtField* f = klass->GetInstanceField(i);
      if (IsReferenceField(f)) {
        Primitive::Type type = f->GetTypeAsPrimitiveType();
        if (type == Primitive::kPrimBoolean) {
          __ AddU4(f->GetBoolean(obj));
        } else {
          __ AddU4(f->Get32(obj));
        }
      }
    }
    if (AddRuntimeInternalObjectsField(klass)) {
      // We need an id that is guaranteed to not be used, use 1/2 of the object alignment.
      fake_object_array = reinterpret_cast<mirror::Object*>(
          reinterpret_cast<uintptr_t>(obj) + kObjectAlignment / 2);
      __ AddObjectId(fake_object_array);
    }
    klass = klass->GetSuperClass().Ptr();
  } while (klass != nullptr);

  // Patch the instance field length.
  __ UpdateU4(size_patch_offset, output_->Length() - (size_patch_offset + 4));

  instance_length_ += output_->Length() - bytes_copy_start;

  if (fake_object_array != nullptr) {
    DumpFakeObjectArray(fake_object_array, fake_roots);
  }

  ++objects_in_segment_;
  ++instance_count_;
}

void StripHprof::VisitRoot(mirror::Object* obj, const RootInfo& info) {
  static const HprofHeapTag xlate[] = {
    HPROF_ROOT_UNKNOWN,
    HPROF_ROOT_JNI_GLOBAL,
    HPROF_ROOT_JNI_LOCAL,
    HPROF_ROOT_JAVA_FRAME,
    HPROF_ROOT_NATIVE_STACK,
    HPROF_ROOT_STICKY_CLASS,
    HPROF_ROOT_THREAD_BLOCK,
    HPROF_ROOT_MONITOR_USED,
    HPROF_ROOT_THREAD_OBJECT,
    HPROF_ROOT_INTERNED_STRING,
    HPROF_ROOT_FINALIZING,
    HPROF_ROOT_DEBUGGER,
    HPROF_ROOT_REFERENCE_CLEANUP,
    HPROF_ROOT_VM_INTERNAL,
    HPROF_ROOT_JNI_MONITOR,
  };
  CHECK_LT(info.GetType(), sizeof(xlate) / sizeof(HprofHeapTag));
  if (obj == nullptr) {
    return;
  }
  MarkRootObject(obj, xlate[info.GetType()], info.GetThreadId());
}

void StripHprof::MarkRootObject(const mirror::Object* obj, HprofHeapTag heap_tag, uint32_t thread_serial) {
  if (heap_tag == 0) {
    return;
  }

  CheckHeapSegmentConstraints();

  switch (heap_tag) {
    case HPROF_ROOT_STICKY_CLASS:
    case HPROF_ROOT_MONITOR_USED:
    case HPROF_ROOT_JNI_GLOBAL:
    case HPROF_ROOT_JNI_LOCAL:
    case HPROF_ROOT_JNI_MONITOR:
    case HPROF_ROOT_JAVA_FRAME:
    case HPROF_ROOT_NATIVE_STACK:
    case HPROF_ROOT_THREAD_BLOCK:
    case HPROF_ROOT_THREAD_OBJECT: {
      uint64_t key = (static_cast<uint64_t>(heap_tag) << 32) | PointerToLowMemUInt32(obj);
      if (simple_roots_.insert(key).second) {
        __ AddU1(heap_tag);
        __ AddObjectId(obj);

        if (heap_tag == HPROF_ROOT_JAVA_FRAME || heap_tag == HPROF_ROOT_THREAD_OBJECT) {
          __ AddU4(thread_serial);
        }

        ++objects_in_segment_;
      }
      break;
    }
    default: return;
  }
}

void DumpStripHeap(const char* filename) {
  CHECK(filename != nullptr);
  Thread* self = Thread::Current();
  // Need to take a heap dump while GC isn't running. See the comment in Heap::VisitObjects().
  // Also we need the critical section to avoid visiting the same object twice. See b/34967844
  gc::ScopedGCCriticalSection gcs(self,
                                  gc::kGcCauseHprof,
                                  gc::kCollectorTypeHprof);
  ScopedSuspendAll ssa(__FUNCTION__, true /* long suspend */);
  StripHprof hprof(filename);
  hprof.Dump();
}

void ForkDumpHeap(const char* filename, bool strip) NO_THREAD_SAFETY_ANALYSIS {
  CHECK(filename != nullptr);
  Thread* self = Thread::Current();
  std::optional<gc::ScopedGCCriticalSection> gcs(std::in_place, self, gc::kGcCauseHprof, gc::kCollectorTypeHprof);
  std::optional<ScopedSuspendAll> ssa(std::in_place, __FUNCTION__, true /* long suspend */);
  Locks::mutator_lock_->ExclusiveUnlock(self);
  gcs.reset();

  pid_t pid = fork();
  if (pid == 0) {
    // Set timeout for child process
    alarm(60);
    prctl(PR_SET_NAME, "forked-dump-process");
    if (strip) {
      DumpStripHeap(filename);
    } else {
      DumpHeap(filename, -1, false);
    }
    _exit(0);
  } else if (pid > 0) {
    Locks::mutator_lock_->ExclusiveLock(self);
    ssa.reset();
    int status;
    if (waitpid(pid, &status, 0) != -1 || errno != EINTR) {
      if (!WIFEXITED(status)) {
        LOG(INFO) << "Child process " << pid
                  << " exited with status " << WEXITSTATUS(status)
                  << ", terminated by signal " << WTERMSIG(status);
      }
    }
  }
}

} // namespace hprof
} // namespace art