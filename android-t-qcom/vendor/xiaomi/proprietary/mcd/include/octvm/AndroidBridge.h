#ifndef OCTVM_ANDROIDRUNTIME_H
#define OCTVM_ANDROIDRUNTIME_H

namespace android {
/*
 *dump bridge of android runtime
 */
int32_t system_info_dump(char *service_name);
int32_t dumpstate_silent(void);

};

#endif
