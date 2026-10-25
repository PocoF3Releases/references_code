#ifndef OCTVM_DEBUG_UTILS_H
#define OCTVM_DEBUG_UTILS_H

#include <stdbool.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#if __LP64__
#define HIGH_ADDR 0x4000000000000000UL
#define PRIPTR "016" PRIxPTR
#define PRINADATA "---------------- "
#else
#define HIGH_ADDR 0xffff0000
#define PRIPTR "08" PRIxPTR
#define PRINADATA "-------- "
#endif

#define GLOBAL_MEMINFO   0x1
#define PRIV_HEAP_INFO   0x2
#define CUR_STACK_DATA   0x4

void dump_self_memory_info(int fd);
void dump_global_memory_info(int fd);
void dump_current_stack_data(int fd);
void log_debug_info(int debug_flag, char *message, char *calling_func);

bool get_runtime_debug_state(void);
void set_runtime_debug_state(bool state);

bool checkDebug();

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

#endif //OCTVM_DEBUG_UTILS_H
