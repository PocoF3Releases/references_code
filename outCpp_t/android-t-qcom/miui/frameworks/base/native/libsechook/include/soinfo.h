#ifndef __INJECTOR_H__
#define __INJECTOR_H__

#include <elf.h>
#include <dlfcn.h>
#include <unistd.h>
#include <link.h>

#define SOINFO_NAME_LEN 128

#define FLAG_LINKED     0x00000001
#define FLAG_EXE        0x00000004 // The main executable
#undef ANDROID_SH_LINKER
#define ANDROID_ARM_LINKER

// Android uses RELA for aarch64 and x86_64. mips64 still uses REL.
#if defined(__aarch64__) || defined(__x86_64__)
    #define USE_RELA 1
#endif

#if __LP64__
    #define ELFH(type) ELF64_ ## type
#else
    #define ELFH(type) ELF32_ ## type
#endif

#if defined(USE_RELA)
    #define GET_REL(what) what->rela
    #define GET_REL_COUNT(what) what->rela_count
    #define GET_PLT_REL(what) what->plt_rela
    #define GET_PLT_REL_COUNT(what) what->plt_rela_count
#else
    #define GET_REL(what) what->rel
    #define GET_REL_COUNT(what) what->rel_count
    #define GET_PLT_REL(what) what->plt_rel
    #define GET_PLT_REL_COUNT(what) what->plt_rel_count
#endif


#if defined(__LP64__)

    #define ELF(what) Elf64_ ## what
    #define ELF_R_SYM ELF64_R_SYM

    #if defined(USE_RELA)
        #define ELF_REL Elf64_Rela
    #else
        #define ELF_REL Elf64_Rel
    #endif

    typedef unsigned long ptr_t;

#else //#if defined(__LP64__)

    #define ELF(what) Elf32_ ## what
    #define ELF_R_SYM ELF32_R_SYM

    #ifdef ANDROID_SH_LINKER
        #define ELF_REL Elf32_Rela
    #else
        #define ELF_REL Elf32_Rel
    #endif

    typedef unsigned ptr_t;

#endif //#if defined(__LP64__)


typedef void (*linker_function_t)();

struct soinfo {
#if !defined(PLATFORM_VERSION_GREATER_THAN_22) || !defined(__LP64__)
    char name[SOINFO_NAME_LEN];
#endif
    const ELF(Phdr)* phdr;
    size_t phnum;
    ptr_t entry;
    ptr_t base;
    size_t size;

#ifndef __LP64__
    uint32_t unused1;  // DO NOT USE, maintained for compatibility.
#endif

    ptr_t* dynamic;

#ifndef __LP64__
    uint32_t unused2; // DO NOT USE, maintained for compatibility
    uint32_t unused3; // DO NOT USE, maintained for compatibility
#endif

    soinfo* next;
    uint32_t flags;

    const char* strtab;
    ELF(Sym)* symtab;

    size_t nbucket;
    size_t nchain;
    uint32_t* bucket;
    uint32_t* chain;

#if defined(__mips__) || !defined(__LP64__)
    // This is only used by mips and mips64, but needs to be here for
    // all 32-bit architectures to preserve binary compatibility.
    ELF(Addr)** plt_got;
#endif

#if defined(USE_RELA)
    ELF(Rela)* plt_rela;
    size_t plt_rela_count;

    ELF(Rela)* rela;
    size_t rela_count;
#else
    ELF(Rel)* plt_rel;
    size_t plt_rel_count;

    ELF(Rel)* rel;
    size_t rel_count;
#endif

    linker_function_t* preinit_array;
    size_t preinit_array_count;

    linker_function_t* init_array;
    size_t init_array_count;
    linker_function_t* fini_array;
    size_t fini_array_count;

    linker_function_t init_func;
    linker_function_t fini_func;

#if defined(__arm__)
    // ARM EABI section used for stack unwinding.
    uint32_t* ARM_exidx;
    size_t ARM_exidx_count;
#elif defined(__mips__)
    uint32_t mips_symtabno;
    uint32_t mips_local_gotno;
    uint32_t mips_gotsym;
#endif

    size_t refcount;

#ifdef PLATFORM_VERSION_GREATER_THAN_20
    link_map link_map_head;
#else
    struct link_map_t {
      uintptr_t l_addr;
      char*  l_name;
      uintptr_t l_ld;
      link_map_t* l_next;
      link_map_t* l_prev;
    } link_map_head;
#endif

    bool constructors_called;

    // When you read a virtual address from the ELF file, add this
    // value to get the corresponding address in the process' address space.
    ELF(Addr) load_bias;
};

#endif // ifndef __INJECTOR_H__