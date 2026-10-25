#ifndef OCTVM_MEMSW_STATE_H
#define OCTVM_MEMSW_STATE_H

#include <stdio.h>
#include <sys/ioctl.h>
#include <cutils/log.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define MEMSW_STATE_DEV        "xiaomi_memsw_state"
#define MEMSW_STATE_DEV_SHORT  "memsw_state"

#define __MEMSWIO   0xAE

#define GET_MEM_THRESHOLD       _IO(__MEMSWIO, 0x11)
#define SET_MEM_THRESHOLD       _IO(__MEMSWIO, 0x12)
#define GET_SWAP_THRESHOLD      _IO(__MEMSWIO, 0x13)
#define SET_SWAP_THRESHOLD      _IO(__MEMSWIO, 0x14)
#define GET_MEMSW_DEV_VERSION   _IO(__MEMSWIO, 0x15)
#define SET_MEMSW_DEV_VERSION   _IO(__MEMSWIO, 0x16)

#define MEMSW_STATE_POLL_TIMEOUT  5000

/*
 * event from memsw_state dev:
 * indicating memory and swap is reaching the threshold
 */
struct memsw_state_data {
    uint8_t low_mem_triggered;
    uint8_t low_swap_triggered;
    unsigned long mem_threshold;
    unsigned long swap_threshold;
    unsigned long current_freeram;
    unsigned long current_freeswap;
};

int memsw_state_device_init();
void memsw_state_event_deinit();
int memsw_state_event_init(long *mem_kB, long *swap_kB);
int memsw_state_get_threshold(long *mem_kB, long *swap_kB);
int memsw_state_set_threshold(long *mem_kB, long *swap_kB);
int poll_memsw_state_event(struct memsw_state_data *target_event_data);

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

#endif
