#ifndef _BACKTRACE_FAST_COMPACT_UNWIND_CURRENT_H
#define _BACKTRACE_FAST_COMPACT_UNWIND_CURRENT_H

#include <inttypes.h>
#include <stdint.h>
#include <sys/types.h>
#include <string>
#include <vector>

#include <backtrace/Backtrace.h>
#include <backtrace/BacktraceMap.h>

struct fast_compact_backtrace_frame_data_t {
  uintptr_t pc;           // The absolute pc.
  uintptr_t sp;           // The top of the stack.
};

class FastCompactUnwindCurrent {
public:
  FastCompactUnwindCurrent(int32_t max_depth);
  ~FastCompactUnwindCurrent();

  void Reset();

  // Get the current stack trace and store in the backtrace_ structure.
  bool Unwind();

  FastCompactUnwindCurrent* clone();

  int32_t NumFrames() const { return num_frames_; }

  const fast_compact_backtrace_frame_data_t* GetFrame(int32_t frame_num) {
    if (frame_num >= num_frames_) {
      return NULL;
    }
    return frames_ + frame_num;
  }

  BacktraceUnwindError GetError() { return error_; }

  std::string GetErrorString(BacktraceUnwindError error);

  // Create a string representing the formatted line of backtrace information
  // for a single frame.
  std::string FormatFrameData(int32_t frame_num, BacktraceMap* map);
  std::string FormatFrameData(const fast_compact_backtrace_frame_data_t* frame, BacktraceMap* map);

  void FormatFrames(BacktraceMap* map, std::vector<std::string>& result);

private:
  // Get the function name and offset into the function given the pc.
  // If the string is empty, then no valid function name was found.
  std::string GetFunctionName(uintptr_t pc, uintptr_t* offset);

private:
  void* context_;
  int32_t max_depth_;
  fast_compact_backtrace_frame_data_t* frames_;
  int32_t num_frames_;
  BacktraceUnwindError error_;
};

#endif // _BACKTRACE_FAST_COMPACT_UNWIND_CURRENT_H
