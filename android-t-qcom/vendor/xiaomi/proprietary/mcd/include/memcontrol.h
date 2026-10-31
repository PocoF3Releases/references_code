#ifndef MEMCONTROL_H
#define MEMCONTROL_H

#include <cutils/log.h>

#include "octvm.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory control API: not recommend to directly use them

typedef enum{
    TMC_MODE_DEFAULT,
    TMC_MODE_MORE_SWAP = TMC_MODE_DEFAULT,
    TMC_MODE_MORE_MEMORY
}MemorySwapMode;

#define TMC_OPERATION_MAX_TASK_NUM  128

/*
 * call from whetstone
 * this function will change values of swappiness, limit and soft_limit.
 * flag is reserved, for future use.
 */
int tmc_mode_enter(MemorySwapMode mode, int flag);

/*
 * call from whetstone
 * 1, open the memsw_state device
 * 2, set the memory limit threshold
 * 3, set the swap limit threshold
 * this return the memsw_state fd, if return negative, means one of the above failed
 */
int tmc_applications_low_memsw_event_init(long mem_KB, long swap_KB);

/*
 * call from whetstone
 * this function will update  values of low memory threshold.
 */
int tmc_applications_update_low_memory_threshold(long mem_kB, long swap_kB);

/*
 * call from whetstone
 * this function will call poll on the memsw_state device,
 * it means this function will blocking here unless there one threshold is reached
 * this function should return with code positive, it returns mean there something
 * need to do because the threshold is reached.
 */
int tmc_applications_low_memsw_event_wait(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

#endif
