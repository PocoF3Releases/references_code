#ifndef MC_SECURITY_H
#define MC_SECURITY_H

#include <linux/capability.h>

struct _mc_cap_t {
    struct __user_cap_header_struct capheader;
    struct __user_cap_data_struct capdata[2];
};
typedef struct _mc_cap_t mc_cap_t;

int mc_switch_system_user(void);
int mc_get_my_cap(mc_cap_t *result);
int mc_get_pid_cap(int pid, mc_cap_t *result);
int mc_print_my_cap(void);

#endif
