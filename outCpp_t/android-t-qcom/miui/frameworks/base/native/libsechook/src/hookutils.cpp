#define ALOGD(...)
#define ALOGI(...)
#define LOG_TAG "LIBSEC"
#include <utils/Log.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <utils/SortedVector.h>
#include <nativehelper/JNIHelp.h>

#include "include/socket_hook.h"
#include "include/hookutils.h"

using namespace android;

#ifdef PLATFORM_VERSION_GREATER_THAN_22
    #define PTHREAD_RECURSIVE_MUTEX_INITIALIZER PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#endif //PLATFORM_VERSION_GREATER_THAN_22

static pthread_mutex_t lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;

#define LIBDL_NAME "libdl.so"

static hook_symbol_t* hijacked_symbol;

static int hook_symbol_count;

static soinfo* somain = NULL;

static void* (*real_dlopen_ptr)(const char*, int) = (void* (*)(const char*, int))get_fun_ptr_dlsym("dlopen");
static void* (*real_dlsym_ptr)(void*, const char*) = (void* (*)(void*, const char*))get_fun_ptr_dlsym("dlsym");
static int (*real_dladdr_ptr)(const void*, Dl_info*) = (int (*)(const void*, Dl_info*))get_fun_ptr_dlsym("dladdr");
static int (*real_dlclose_ptr)(void*) = (int (*)(void*))get_fun_ptr_dlsym("dlclose");

#ifdef PLATFORM_VERSION_GREATER_THAN_20
static void* (*real_android_dlopen_ext_ptr)(const char*, int, const android_dlextinfo*) =
    (void* (*)(const char*, int, const android_dlextinfo*))get_fun_ptr_dlsym("android_dlopen_ext");
#endif //PLATFORM_VERSION_GREATER_THAN_20

#ifdef PLATFORM_VERSION_GREATER_THAN_23
static void (*android_set_application_target_sdk_version_ptr)(uint32_t target) =
    (void (*)(uint32_t target))get_fun_ptr_dlsym("android_set_application_target_sdk_version");
static uint32_t (*android_get_application_target_sdk_version_ptr)() =
    (uint32_t (*)())get_fun_ptr_dlsym("android_get_application_target_sdk_version");
static soinfo* wrapped_dlopen(const char* path, int mode) {
    uint32_t real_sdk_version = android_get_application_target_sdk_version_ptr();
    android_set_application_target_sdk_version_ptr(23);
    soinfo* so_ptr = (soinfo*) real_dlopen_ptr(path, mode);
    android_set_application_target_sdk_version_ptr(real_sdk_version);
    return so_ptr;
}
static soinfo* wrapped_android_dlopen_ext(const char* path, int mode, const android_dlextinfo* info) {
    uint32_t real_sdk_version = android_get_application_target_sdk_version_ptr();
    android_set_application_target_sdk_version_ptr(23);
    soinfo* so_ptr = (soinfo*) real_android_dlopen_ext_ptr(path, mode, info);
    android_set_application_target_sdk_version_ptr(real_sdk_version);
    return so_ptr;
}
#else //PLATFORM_VERSION_GREATER_THAN_23
static soinfo* wrapped_dlopen(const char* path, int mode) {
    return (soinfo*) real_dlopen_ptr(path, mode);
}

#ifdef PLATFORM_VERSION_GREATER_THAN_20
static soinfo* wrapped_android_dlopen_ext(const char* path, int mode, const android_dlextinfo* info) {
    return (soinfo*) real_android_dlopen_ext_ptr(path, mode, info);
}
#endif //PLATFORM_VERSION_GREATER_THAN_20
#endif //PLATFORM_VERSION_GREATER_THAN_23

static soinfo* get_so_main() {
    return wrapped_dlopen(LIBDL_NAME, RTLD_NOW);
}

static hook_symbol_t LINKER_HOOKED_SYMBOLS[] = {
    { "dlopen", (ptr_t) __dlopen, (ptr_t) real_dlopen_ptr },
    { "dlsym", (ptr_t) __dlsym, (ptr_t) real_dlsym_ptr },
    { "dladdr", (ptr_t) __dladdr, (ptr_t) real_dladdr_ptr },
    { "dlclose", (ptr_t) __dlclose, (ptr_t) real_dlclose_ptr },
#ifdef PLATFORM_VERSION_GREATER_THAN_20
    { "android_dlopen_ext", (ptr_t) __android_dlopen_ext, (ptr_t) real_android_dlopen_ext_ptr },
#endif //PLATFORM_VERSION_GREATER_THAN_20
};

static int LINKER_HOOKED_SYMBOLS_COUNT = NELEM(LINKER_HOOKED_SYMBOLS);


static inline ptr_t get_so_base(soinfo *so) {
    ptr_t base_ptr = so->load_bias;
    if (so->flags & FLAG_EXE) {
        for (size_t nn = 0; nn < (unsigned)so->phnum; nn++) {
            if (so->phdr[nn].p_type == PT_PHDR) {
                base_ptr = (ELF(Addr)) so->phdr - so->phdr[nn].p_vaddr;
                break;
            }
        }
    }
    return base_ptr;
}

static void do_get_sections(soinfo *so, ELF_REL **rel, size_t *rel_count,
        ELF_REL **plt_rel, size_t *plt_rel_count) {
    if ((GET_REL(so) != NULL && GET_REL_COUNT(so) != 0)
            || (GET_PLT_REL(so) != NULL && GET_PLT_REL_COUNT(so) != 0)) {
        *rel = GET_REL(so);
        *rel_count = GET_REL_COUNT(so);
        *plt_rel = GET_PLT_REL(so);
        *plt_rel_count = GET_PLT_REL_COUNT(so);

        return;
    }

    if (so->dynamic == NULL) {
        return;
    }

    for (ptr_t *d = so->dynamic; *d; d++) {
        switch (*d++) {
        case DT_REL:
            *rel = (ELF_REL *) (so->base + *d);
            break;

        case DT_RELSZ:
            *rel_count = *d / 8;
            break;

        case DT_JMPREL:
            *plt_rel = (ELF_REL *) (so->base + *d);
            break;

        case DT_PLTRELSZ:
            *plt_rel_count = *d / 8;
            break;
        }
    }
}

static bool validate_so(soinfo *so) {
    if (so->base == 0)
        return false;
    if (so->refcount == 0)
        return false;
    #ifdef PLATFORM_VERSION_GREATER_THAN_25
    if (msync((void *) so->base, so->size, MS_ASYNC) < 0)
        return false;
    #else
    if (msync((const void *) so->base, so->size, MS_ASYNC) < 0)
        return false;
    #endif
    if ((so->flags & FLAG_LINKED) == 0)
        return false;
    return true;
}

static int do_set_writable(ptr_t location) {
    static size_t pagesize = 0;
    if (pagesize == 0) {
        pagesize = sysconf(_SC_PAGESIZE);
    }
    return mprotect((void *) (location & (((size_t) -1) ^ (pagesize - 1))),
            pagesize, PROT_READ | PROT_WRITE);
}

ptr_t get_fun_ptr_dlsym(const char *funcname) {
    return (ptr_t)dlsym(RTLD_DEFAULT, funcname );
}

static bool is_self(soinfo *so) {
    ptr_t ptr = (ptr_t) is_self;
    return (ptr >= so->base && ptr <= so->base + so->size);
}

static void do_replace_import(soinfo *so, ELF_REL *rel, size_t count,
        const char *name, ptr_t new_ptr, ptr_t old_ptr) {
    ELF(Sym) *symtab = so->symtab;
    const char *strtab = so->strtab;
    size_t idx;
    ALOGD("so=0x%08X rel=0x%08X count=%d", so, rel, count);
    ptr_t base = get_so_base(so);

    if (rel == NULL || symtab == NULL || strtab == NULL)
        return;

    for (idx = 0; idx < count; idx++) {
        ptr_t sym = ELF_R_SYM(rel[idx].r_info);
        ptr_t reloc = (ptr_t) (rel[idx].r_offset + base);
        char *sym_name = NULL;
    #ifndef __LP64__
        const char *so_name = so->name;
    #else
        const char *so_name = "noname";
    #endif

        if (sym != 0) {
            sym_name = (char *) (strtab + symtab[sym].st_name);
            ALOGD("sym_name=%s to compare with name=%s", sym_name, name);
            if (strcmp(sym_name, name) == 0) {
                ALOGD("change %s %s 0x%08X from 0x%08X to 0x%08X", so_name, sym_name, reloc, *((ptr_t*)reloc), new_ptr);
                if (do_set_writable(reloc) == 0) {
                    ELF(Word) type = ELFH(R_TYPE)(rel[idx].r_info);
                    switch(type) {
                #if defined(__arm__)
                    case R_ARM_ABS32:
                    case R_ARM_REL32:
                        break;
                #elif defined(__aarch64__)
                    case R_AARCH64_ABS64:
                    case R_AARCH64_ABS32:
                    case R_AARCH64_ABS16:
                    case R_AARCH64_PREL64:
                    case R_AARCH64_PREL32:
                    case R_AARCH64_PREL16:
                #endif
                        *((ptr_t*) reloc) += (new_ptr - old_ptr);
                        break;
                    default:
                        *((ptr_t*) reloc) = new_ptr;
                        break;
                    }
                } else {
                    ALOGE("sym_name=%s, set_writable failed", sym_name);
                }
            }
        }
    }
}

static void do_hook(const char *filename __unused, soinfo *result, SortedVector<soinfo *>* previous) {
    if (result != NULL) {
        for (soinfo *so = result; so; so = so->next) {
            if (!validate_so(so) || is_self(so) || (previous != NULL && (*previous).indexOf(so) >= 0))
                continue;
            ALOGD("do_hook %s, Process so %s, base 0x%08X, size 0x%08X", filename, "so->name", so->base, so->size);
            ELF_REL *rel = NULL, *plt_rel = NULL;
            size_t rel_count = 0, plt_rel_count = 0;
            do_get_sections(so, &rel, &rel_count, &plt_rel, &plt_rel_count);

            for (int i = 0; i < LINKER_HOOKED_SYMBOLS_COUNT; i++) {
                do_replace_import(so, plt_rel, plt_rel_count,
                        LINKER_HOOKED_SYMBOLS[i].symbol, LINKER_HOOKED_SYMBOLS[i].ptr, LINKER_HOOKED_SYMBOLS[i].old_ptr);
                do_replace_import(so, rel, rel_count, LINKER_HOOKED_SYMBOLS[i].symbol,
                        LINKER_HOOKED_SYMBOLS[i].ptr, LINKER_HOOKED_SYMBOLS[i].old_ptr);
            }

            for (int i = 0; i < hook_symbol_count; i++) {
                do_replace_import(so, plt_rel, plt_rel_count,
                        hijacked_symbol[i].symbol, hijacked_symbol[i].ptr, hijacked_symbol[i].old_ptr);
                do_replace_import(so, rel, rel_count, hijacked_symbol[i].symbol,
                        hijacked_symbol[i].ptr, hijacked_symbol[i].old_ptr);
            }
        }
    }
}

static void append_do_hook(const char *filename __unused, soinfo *result, SortedVector<soinfo *>* previous,
    hook_symbol_t *symbols, int count) {
    ALOGI("append_do_hook");
    if (result != NULL) {
        for (soinfo *so = result; so; so = so->next) {
            if (!validate_so(so) || is_self(so) || (previous != NULL && (*previous).indexOf(so) >= 0))
                continue;
            ALOGD("append_do_hook %s, Process so %s, base 0x%08X, size 0x%08X", filename, "so->name", so->base, so->size);
            ELF_REL *rel = NULL, *plt_rel = NULL;
            size_t rel_count = 0, plt_rel_count = 0;
            do_get_sections(so, &rel, &rel_count, &plt_rel, &plt_rel_count);

            for (int i = 0; i < count; i++) {
                do_replace_import(so, plt_rel, plt_rel_count,
                        symbols[i].symbol, symbols[i].ptr, symbols[i].old_ptr);
                do_replace_import(so, rel, rel_count, symbols[i].symbol,
                        symbols[i].ptr, symbols[i].old_ptr);
            }
        }
    }
}

static void* __dlsym(void* handle, const char* symbol) {
    ALOGI("__dlsym called, symbol:%s", symbol);
    pthread_mutex_lock(&lock);
    void* result = real_dlsym_ptr(handle, symbol);
    pthread_mutex_unlock(&lock);
    return result;
}

static int __dladdr(const void* addr, Dl_info* info) {
    ALOGI("__dladdr called");
    pthread_mutex_lock(&lock);
    int result = real_dladdr_ptr(addr, info);
    pthread_mutex_unlock(&lock);
    return result;
}

static int __dlclose(void* handle) {
    ALOGI("__dlclose called");
    pthread_mutex_lock(&lock);
    int result = real_dlclose_ptr(handle);
    pthread_mutex_unlock(&lock);
    return result;
}

static void *__dlopen(const char *filename, int flag) {
    ALOGI("__dlopen called, filename: %s", filename);
    ALOGD("__dlopen called, filename: %s", filename);

    pthread_mutex_lock(&lock);

    SortedVector<soinfo *> previous;
    for (soinfo *so = somain; so; so = so->next) {
        previous.add(so);
    }

    soinfo *result = (soinfo *) wrapped_dlopen(filename, flag);
    do_hook(filename, result, &previous);

    pthread_mutex_unlock(&lock);
    return result;
}

#ifdef PLATFORM_VERSION_GREATER_THAN_20
static void *__android_dlopen_ext(const char *filename, int flag, const android_dlextinfo* extinfo) {
    ALOGI("__android_dlopen_ext called, filename: %s", filename);
    ALOGD("__android_dlopen_ext called, filename: %s", filename);
    pthread_mutex_lock(&lock);

    ALOGI("__android_dlopen_ext %s", filename);
    SortedVector<soinfo *> previous;
    for (soinfo *so = somain; so; so = so->next) {
        previous.add(so);
    }

    soinfo *result = (soinfo *) wrapped_android_dlopen_ext(filename, flag, extinfo);
    do_hook(filename, result, &previous);

    pthread_mutex_unlock(&lock);
    return result;
}
#endif

ptr_t hook_symbol(const char *soname, const char *symbol, const char *victim,
        ptr_t new_symbol, ptr_t old_symbol) {
    if (somain == NULL) {
        somain = get_so_main();
    }
    ALOGD("hook_symbol: soname=%s symbol=%s victim=%s", soname, symbol, victim);
    soinfo *dest = (soinfo *) wrapped_dlopen(soname, RTLD_NOW);
    ptr_t result = dest ? (ptr_t) dlsym(dest, symbol) : 0;
    if (!result) {
        ALOGD("unable to find %s", symbol);
        return result;
    }

    soinfo *so = NULL;
    if (victim) {
        so = (soinfo *) wrapped_dlopen(victim, RTLD_NOW);
    } else {
        so = somain;
    }

    if (so == NULL) {
        ALOGD("unable to load %s", victim);
        return 0;
    }

    ELF_REL *rel = NULL, *plt_rel = NULL;
    size_t rel_count = 0, plt_rel_count = 0;
    do_get_sections(so, &rel, &rel_count, &plt_rel, &plt_rel_count);
    do_replace_import(so, plt_rel, plt_rel_count, symbol, new_symbol, old_symbol);
    do_replace_import(so, rel, rel_count, symbol, new_symbol, old_symbol);

    return result;
}

void hook_symbol_recursive(hook_symbol_t *symbols, int count) {
    hijacked_symbol = symbols;
    hook_symbol_count = count;

    if (somain == NULL) {
        somain = get_so_main();
    }
    do_hook(LIBDL_NAME, somain, NULL);
}

void append_hook_symbol_recursive(hook_symbol_t *symbols, int count) {
    if (somain == NULL) {
        somain = get_so_main();
    }

    hook_symbol_t  copied_table[count];
    int copied_count = 0;
    for (int i = 0; i < count; i++) {
        int copy = 1;
        for (int j = 0; j < LINKER_HOOKED_SYMBOLS_COUNT; j++) {
            ALOGI("cmp %s==%s", LINKER_HOOKED_SYMBOLS[j].symbol, symbols[i].symbol);
            if(!strcmp(LINKER_HOOKED_SYMBOLS[j].symbol, symbols[i].symbol)) {
                ALOGE("repeated %s, skip", symbols[i].symbol);
                copy = 0;
                break;
            }
        }
        for (int j = 0; j < hook_symbol_count; j++) {
            ALOGI("cmp %s==%s", hijacked_symbol[j].symbol, symbols[i].symbol);
            if(!strcmp(hijacked_symbol[j].symbol, symbols[i].symbol)) {
                ALOGE("repeated %s, skip", symbols[i].symbol);
                copy = 0;
                break;
            }
        }
        if(copy) {
           copied_table[copied_count++] = symbols[i];
        }
    }
    append_do_hook(LIBDL_NAME, somain, NULL, copied_table, copied_count);
}


static ptr_t find_symbol(soinfo *so, ELF_REL *rel, size_t count, const char *name) {
    ELF(Sym) *symtab = so->symtab;
    const char *strtab = so->strtab;
    size_t idx;
    ALOGD("so=0x%08X rel=0x%08X count=%d\n", so, rel, count);
    ptr_t base = get_so_base(so);

    if (rel == NULL || symtab == NULL || strtab == NULL)
        return 0;

    for (idx = 0; idx < count; idx++) {
        ptr_t sym = ELF_R_SYM(rel[idx].r_info);
        ptr_t reloc = (ptr_t) (rel[idx].r_offset + base);
        char *sym_name = NULL;
        if (sym != 0) {
            sym_name = (char *) (strtab + symtab[sym].st_name);
            ALOGD("sym_name=%s to compare with name=%s\n", sym_name, name);
            if (strcmp(sym_name, name) == 0) {
                ALOGD("find it XXXXXXXX  addr:%lx, value:%lx\n", reloc, *((ptr_t*) reloc));
                return *((ptr_t*) reloc);
            }
        }
    }

    return 0;
}

static ptr_t find_symbol_in_library(const char *libname, const char *symname) {
    somain = wrapped_dlopen(libname, RTLD_NOW);
    if (somain != NULL) {
        for (soinfo *so = somain; so; so = so->next) {
            if (!validate_so(so) || is_self(so))
                continue;
            ALOGD("do_hook %s, base 0x%08X, size 0x%08X\n", "so->name", so->base, so->size);
            ELF_REL *rel = NULL, *plt_rel = NULL;
            size_t rel_count = 0, plt_rel_count = 0;
            do_get_sections(so, &rel, &rel_count, &plt_rel, &plt_rel_count);

                ptr_t ret;
                ret = find_symbol(so, plt_rel, plt_rel_count, symname);
                if(ret > 0) {
                    ALOGD("find it ret:%lx\n", ret);
                    return ret;
                }
                ret = find_symbol(so, rel, rel_count, symname);
                if(ret > 0) {
                    ALOGD("find it ret:%lx\n", ret);
                    return ret;
                }
        }
    }
    return 0;
}
