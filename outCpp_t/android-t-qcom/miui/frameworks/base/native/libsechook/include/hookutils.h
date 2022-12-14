#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "soinfo.h"

#ifdef PLATFORM_VERSION_GREATER_THAN_20
#include <android/dlext.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hook_symbol_t {
    const char *symbol;
    ptr_t ptr;
    ptr_t old_ptr;
} hook_symbol_t;

ptr_t hook_symbol(const char *soname, const char *symbol, const char *victim, ptr_t new_symbol);

void hook_symbol_recursive(hook_symbol_t *symbols, int count);

void append_hook_symbol_recursive(hook_symbol_t *symbols, int count);

ptr_t get_fun_ptr_dlsym(const char *funcname);

static void* __dlsym(void* handle, const char* symbol);

static int __dladdr(const void* addr, Dl_info* info);

static int __dlclose(void* handle);

static void *__dlopen(const char *filename, int flag);

#ifdef PLATFORM_VERSION_GREATER_THAN_20
static void *__android_dlopen_ext(const char *filename, int flag, const android_dlextinfo* extinfo);
#endif

#ifdef __cplusplus
}
#endif
