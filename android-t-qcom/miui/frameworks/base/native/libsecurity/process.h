#ifndef _SECURITY_PROCESS_H
#define _SECURITY_PROCESS_H

#include <stdint.h>

#if __cplusplus
extern "C" {
#endif

int kill_processes_with_uid(int uid, const char* pkgName);


#if __cplusplus
} // extern "C"
#endif

#endif // _SECURITY_PROCESS_H
