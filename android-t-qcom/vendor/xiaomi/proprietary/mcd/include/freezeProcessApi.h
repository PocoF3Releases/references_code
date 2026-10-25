#ifndef FREEZEPROCESS_API_H
#define FREEZEPROCESS_API_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define APP_MAX_PID_NUM          256
#define CGROUP_MAX_NAME_CHARS    128

struct tfp_app_entity {
    int uid;
    int pid[APP_MAX_PID_NUM];
    int pidCnt;
};

#define FROZEN_APP_OK                    	      (0)
#define FROZEN_ERROR_NUM_BASE            	      (-100)
#define FROZEN_ERROR_FIND_CGROUP_ERROR   	      (FROZEN_ERROR_NUM_BASE + 1)
#define FROZEN_ERROR_CHECK_APP_STATE_ERROR            (FROZEN_ERROR_NUM_BASE + 2)
#define FROZEN_ERROR_APP_NULL_ERROR                   (FROZEN_ERROR_NUM_BASE + 3)
#define FROZEN_ERROR_INVALID_ACTION_ERROR             (FROZEN_ERROR_NUM_BASE + 4)
#define FROZEN_ERROR_CGROUP_POOL_FULL_ERROR           (FROZEN_ERROR_NUM_BASE + 5)
#define FROZEN_ERROR_WAKEUP_ITEM_CANNOT_FIND_ERROR    (FROZEN_ERROR_NUM_BASE + 6)
#define FROZEN_CHECK_FROZEN                           (FROZEN_ERROR_NUM_BASE + 7)
#define FROZEN_CHECK_THAWED_THEN_SHOULD_FROZEN        (FROZEN_ERROR_NUM_BASE + 8)
#define FROZEN_CHECK_NO_UID                           (FROZEN_ERROR_NUM_BASE + 9)
#define FROZEN_CHECK_FROZEN_APP_BUT_THAWED_FAIL       (FROZEN_ERROR_NUM_BASE + 10)
#define FROZEN_CHECK_FROZEN_APP_BUT_THAWED_OK         (FROZEN_ERROR_NUM_BASE + 11)
#define FROZEN_CHECK_FROZEN_BUT_BEEN_KILLING          (FROZEN_ERROR_NUM_BASE + 12)
#define FROZEN_CHECK_FROZEN_BUT_BEEN_KILLED           (FROZEN_ERROR_NUM_BASE + 13)

#define FROZEN_APP(pAppEntry)  fp_freeze_app(pAppEntry)
#define THAWED_APP(uid)        fp_thawed_app(uid)

extern int fp_thawed_app(int uid);
extern int fp_freeze_app(struct tfp_app_entity* app);
extern int fp_check_frozen_app_state(int uid);
extern int fp_check_frozen_app_thawed_then_frozen(int uid);

#ifdef __cplusplus
}  /* extern "C" */
#endif
   /* __cplusplus */

#endif
