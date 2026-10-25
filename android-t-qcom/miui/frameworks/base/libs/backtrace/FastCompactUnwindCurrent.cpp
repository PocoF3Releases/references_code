#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <ucontext.h>

#include <android-base/stringprintf.h>

#include <backtrace/FastCompactUnwindCurrent.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

using android::base::StringPrintf;

//-------------------------------------------------------------------------
// FastCompactUnwindCurrent functions.
//-------------------------------------------------------------------------
FastCompactUnwindCurrent::FastCompactUnwindCurrent(int32_t max_depth) :
  context_(NULL), max_depth_(max_depth), frames_(NULL), num_frames_(0),
  error_(BACKTRACE_UNWIND_NO_ERROR) {
  context_ = new unw_context_t;
  frames_ = new fast_compact_backtrace_frame_data_t[max_depth_];
}

FastCompactUnwindCurrent::~FastCompactUnwindCurrent() {
  if (frames_ != NULL) {
    delete[] frames_;
    frames_ = NULL;
  }
  if (context_ != NULL) {
    delete (unw_context_t *)context_;
    context_ = NULL;
  }
}

void FastCompactUnwindCurrent::Reset() {
  num_frames_ = 0;
  error_ = BACKTRACE_UNWIND_NO_ERROR;
}

bool FastCompactUnwindCurrent::Unwind() {
  Reset();

  int ret = unw_getcontext((unw_context_t*)context_);
  if (ret < 0) {
    error_ = BACKTRACE_UNWIND_ERROR_SETUP_FAILED;
    return false;
  }

  // The cursor structure is pretty large, do not put it on the stack.
  std::unique_ptr<unw_cursor_t> cursor(new unw_cursor_t);
  ret = unw_init_local(cursor.get(), (unw_context_t*)context_);
  if (ret < 0) {
    error_ = BACKTRACE_UNWIND_ERROR_SETUP_FAILED;
    return false;
  }

  do {
    unw_word_t pc;
    ret = unw_get_reg(cursor.get(), UNW_REG_IP, &pc);
    if (ret < 0) {
      break;
    }
    unw_word_t sp;
    ret = unw_get_reg(cursor.get(), UNW_REG_SP, &sp);
    if (ret < 0) {
      break;
    }

    fast_compact_backtrace_frame_data_t* frame = frames_ + num_frames_;
    frame->pc = static_cast<uintptr_t>(pc);
    frame->sp = static_cast<uintptr_t>(sp);
    num_frames_++;

    ret = unw_step (cursor.get());
  } while (ret > 0 && num_frames_ < max_depth_);

  return true;
}

FastCompactUnwindCurrent* FastCompactUnwindCurrent::clone() {
  FastCompactUnwindCurrent* cloned = new FastCompactUnwindCurrent(max_depth_);
  memcpy(cloned->context_, context_, sizeof(unw_context_t));
  memcpy(cloned->frames_, frames_, sizeof(fast_compact_backtrace_frame_data_t) * num_frames_);
  cloned->num_frames_ = num_frames_;
  cloned->error_ = error_;
  return cloned;
}

std::string FastCompactUnwindCurrent::GetErrorString(BacktraceUnwindError error) {
  switch (error) {
  case BACKTRACE_UNWIND_NO_ERROR:
    return "No error";
  case BACKTRACE_UNWIND_ERROR_SETUP_FAILED:
    return "Setup failed";
  case BACKTRACE_UNWIND_ERROR_MAP_MISSING:
    return "No map found";
  case BACKTRACE_UNWIND_ERROR_INTERNAL:
    return "Internal libbacktrace error, please submit a bugreport";
  case BACKTRACE_UNWIND_ERROR_THREAD_DOESNT_EXIST:
    return "Thread doesn't exist";
  case BACKTRACE_UNWIND_ERROR_THREAD_TIMEOUT:
    return "Thread has not repsonded to signal in time";
  case BACKTRACE_UNWIND_ERROR_UNSUPPORTED_OPERATION:
    return "Attempt to use an unsupported feature";
  case BACKTRACE_UNWIND_ERROR_NO_CONTEXT:
    return "Attempt to do an offline unwind without a context";
  }
}

std::string FastCompactUnwindCurrent::FormatFrameData(int32_t frame_num, BacktraceMap* map) {
  if (frame_num >= num_frames_) {
    return "";
  }
  return FormatFrameData(frames_ + frame_num, map);
}

static bool isDiscardedFrame(const struct backtrace_map_t& frame_map) {
  if (BacktraceMap::IsValid(frame_map)) {
    const std::string library = basename(frame_map.name.c_str());
    if (library == "libunwind.so" || library == "libbacktrace.so") {
      return true;
    }
  }
  return false;
}

std::string FastCompactUnwindCurrent::FormatFrameData(const fast_compact_backtrace_frame_data_t* frame, BacktraceMap* map) {
  struct backtrace_map_t frame_map;
  if (map != nullptr) {
    map->FillIn(frame->pc, &frame_map);
  }
  if (isDiscardedFrame(frame_map)) {
    return "";
  }

  uintptr_t relative_pc;
  std::string map_name;
  if (BacktraceMap::IsValid(frame_map)) {
    relative_pc = BacktraceMap::GetRelativePc(frame_map, frame->pc);
    if (!frame_map.name.empty()) {
      map_name = frame_map.name.c_str();
      if (map_name[0] == '[' && map_name[map_name.size() - 1] == ']') {
        map_name.resize(map_name.size() - 1);
        map_name += StringPrintf(":%" PRIPTR "]", frame_map.start);
      }
    } else {
      map_name = StringPrintf("<anonymous:%" PRIPTR ">", frame_map.start);
    }
  } else {
    map_name = "<unknown>";
    relative_pc = frame->pc;
  }

  std::string line(StringPrintf("# pc %" PRIPTR "  ", relative_pc));
  line += map_name;
  // Special handling for non-zero offset maps, we need to print that
  // information.
  if (frame_map.offset != 0) {
    line += " (offset " + StringPrintf("0x%" PRIxPTR, frame_map.offset) + ")";
  }
  std::string func_name;
  uintptr_t func_offset;
  func_name = GetFunctionName(frame->pc, &func_offset);
  if (!func_name.empty()) {
    line += " (" + func_name;
    if (func_offset) {
      line += StringPrintf("+%" PRIuPTR, func_offset);
    }
    line += ')';
  }

  return line;
}

void FastCompactUnwindCurrent::FormatFrames(BacktraceMap* map, std::vector<std::string>& result) {
  result.clear();
  if (error_ != BACKTRACE_UNWIND_NO_ERROR || num_frames_ == 0) {
    return;
  }
  for (int i = 0; i < num_frames_; i++) {
    std::string line = FormatFrameData(i, map);
    if (line.length() != 0) {
      result.push_back(line);
    }
  }
}

std::string FastCompactUnwindCurrent::GetFunctionName(uintptr_t pc, uintptr_t* offset) {
  *offset = 0;
  char buf[512];
  unw_word_t value;
  if (unw_get_proc_name_by_ip(unw_local_addr_space, pc, buf, sizeof(buf),
        &value, (unw_context_t*)context_) >= 0 && buf[0] != '\0') {
    *offset = static_cast<uintptr_t>(value);
    return buf;
  }
  return "";
}
