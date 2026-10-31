
#include "init.h"

static struct sys_info si;

static inline struct sys_info *get_sys_info(void)
{
    return &si;
}

static void get_meminfo(struct sys_info *si)
{
    struct sysinfo s;
    int ret = 0;
    unsigned long long total_mem = 0;
    unsigned long long GB = 1024 * 1024 * 1024;
    unsigned long long tmp = 0;
    ret = sysinfo(&s);
    if (ret) {
        ALOGE("call sysinfo() failed: %s\n", strerror(errno));
        return;
    }

    total_mem = (unsigned long long)s.totalram * s.mem_unit;
    if (total_mem <= 4 * GB)
        tmp = GB;
    else if (total_mem > 4 * GB && total_mem <= 10 * GB)
        tmp = 2 * GB;
    else
        tmp = 4 * GB;
    unsigned long long total_mem_BT = (total_mem + tmp -1) & (~(tmp - 1));
    si->total_mem = total_mem_BT / GB;
    return;
}

static void get_datainfo(struct sys_info *si)
{
    struct statfs sfs;
    if(statfs("/data", &sfs) < 0) {
        ALOGE("get data_size failed");
        si->total_data_size = 0;
        return;
    }
    if (sfs.f_blocks == 0) {
        ALOGE("data_size from system is 0");
        si->total_data_size = 0;
        return;
    }

    int data_size_g = sfs.f_bsize * sfs.f_blocks / 1024 / 1024 / 1024;

    // modify the value of data zise to 32,64,128,256,512,1024
    if (data_size_g > 0 && data_size_g < 32)
        si->total_data_size = 32;
    else if (data_size_g >= 32 && data_size_g < 64)
        si->total_data_size = 64;
    else if (data_size_g >= 64 && data_size_g < 128)
        si->total_data_size = 128;
    else if (data_size_g >= 128 && data_size_g < 256)
        si->total_data_size = 256;
    else if (data_size_g >= 256 && data_size_g < 512)
        si->total_data_size = 512;
    else if (data_size_g >= 512 && data_size_g < 1024)
        si->total_data_size = 1024;
    else
        si->total_data_size = data_size_g;

    return;
}

static int parse_cpu_nr(char *line)
{
    char *ptr = NULL;
    int len, index, i, count, first, last, tmp;

    len = strlen(line);
    index = count = first = last = 0;
    for (i = 0; i < len + 1; i++) {
        if (line[i] == ',') {
            line[i] = '\0';

            ptr = strtok(line + index, "-");
            if (ptr) {
                first = atoi(ptr);
                tmp = 1;
            }

            ptr = strtok(NULL, "-");
            if (ptr) {
                last = atoi(ptr);
                tmp = (last - first + 1);
            }

            count += tmp;
            first = last = 0;
            index = i + 1;
        }
    }

    /* handle the last part */
    ptr = strtok(line + index, "-");
    if (ptr) {
        first = atoi(ptr);
        tmp = 1;
    }

    ptr = strtok(NULL, "-");
    if (ptr) {
        last = atoi(ptr);
        tmp = (last - first + 1);
    }
    count += tmp;

    return count;
}

static int get_cpu_nr(const char *filename)
{
    FILE *fp = NULL;
    char line[1024];
    int count = 0;

    fp = fopen(filename, "re");
    if (!fp) {
        ALOGE("Unable to open %s: %s", filename, strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        count += parse_cpu_nr(line);
    }

    fclose(fp);

    return count;
}

static void get_cpuinfo(struct sys_info *si)
{
    si->nr_bg_cpus = get_cpu_nr("/dev/cpuset/background/cpus");
    si->nr_cpus = get_cpu_nr("/sys/devices/system/cpu/present");
}

const static char* malloc_type_to_string(int malloc_type)
{
    switch (malloc_type) {
        case MALLOC_JEMALLOC:
            return "jemalloc";
            break;
        case MALLOC_UNKNOWN:
            return "unkown";
            break;
        default:
            return "unkown";
    }
}

static void print_sysinfo(struct sys_info *si)
{
    ALOGI("       product_name: %s", si->product_name);
    ALOGI("       sdk version:  %d", si->sdk_version);
    ALOGI("       malloc type:  %s", malloc_type_to_string(si->malloc_type));
    ALOGI("         total mem:  %d", si->total_mem);
    ALOGI("        total data:  %d", si->total_data_size);
    ALOGI("   present cpu num:  %d", si->nr_cpus);
    ALOGI("background cpu num:  %d", si->nr_bg_cpus);
}

static void reset_sysinfo(struct sys_info *si) {
    si->product_name = NULL;
    si->sdk_version = SDK_VERSION_UNKNOWN;
    si->malloc_type = MALLOC_UNKNOWN;
    si->total_mem = 0;
    si->nr_cpus = 0;
    si->nr_bg_cpus = 0;
}

struct sys_info *get_sysinfo(void)
{
    struct sys_info *si = get_sys_info();
    reset_sysinfo(si);
    char prop_buf[PROPERTY_VALUE_MAX];

    property_get("ro.build.version.sdk", prop_buf, "");
    if (prop_buf[0] != '\0') {
        si->sdk_version = atoi(prop_buf);
    }

    property_get("ro.malloc.impl", prop_buf, "");
    if (prop_buf[0] != '\0' && strcmp(prop_buf, "jemalloc") == 0) {
        si->malloc_type = MALLOC_JEMALLOC;
    }

    property_get("ro.product.device", prop_buf, "");
    if (prop_buf[0] != '\0') {
        int len = strlen(prop_buf);
        char* model = (char*)malloc(len + 1);
        if (model != NULL) {
            memset(model, 0, len + 1);
            strncpy(model, prop_buf, len);
            si->product_name = model;
        }
    }
    ALOGI("sysinfo: prop_buf-%s,product_name-%s", prop_buf, si->product_name);

    get_meminfo(si);
    get_datainfo(si);
    get_cpuinfo(si);

    print_sysinfo(si);

    return si;
}


