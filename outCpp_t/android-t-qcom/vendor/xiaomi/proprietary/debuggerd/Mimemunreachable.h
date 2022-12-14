#ifndef LIBMIDEBUGGERDIMPL_MIMEMUNREACHABLE_H
#define LIBMIDEBUGGERDIMPL_MIMEMUNREACHABLE_H

#include <memunreachable/memunreachable.h>

namespace android {

bool GetUnreachableMemoryWithoutPtrace(UnreachableMemoryInfo& info, int pid, int input_fd, size_t limit = 100);

void PtraceThreads(int pid, int tid, int output_fd);

}

#endif