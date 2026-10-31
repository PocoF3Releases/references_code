#ifndef _INIT_HOOK_H
#define _INIT_HOOK_H

#ifdef __cplusplus
extern "C" {
#endif

const char* get_cust_prop_file_path();
const char* get_cts_prop_file_path();
const char* get_miui_customized_prop_file_path();
const char* get_carrier_common_prop_file_path();
const char* get_carrier_region_prop_file_path();
const char* get_business_prop_file_path();
char* get_userdata_version_file_path(void);
int should_skip_check(const char* key);

#ifdef __cplusplus
}
#endif

#endif
