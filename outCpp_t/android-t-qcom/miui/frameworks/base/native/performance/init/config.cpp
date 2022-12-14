#include "init.h"
#include <string>
using std::string;

static volatile int available = CONFIG_NON_AVAILABLE;
static struct sys_config sys_conf;

/**
 * Parse a Number cJSON item. cJSON always store the value as a double.
 * The caller is responsible for converting it to a real type.
 * return -1 if any error occurs.
 */
static double parse_number_config(cJSON* parent, const char *name)
{
    double ret = -1;
    cJSON *item = cJSON_GetObjectItem(parent, name);
    if (cJSON_IsNumber(item)) {
        ret = item->valuedouble;
    }
    return ret;
}

/**
 * Parse a String cJSON item. A new buf is allocated to store the value.
 */
static char* parse_string_config(cJSON* parent, const char *name)
{
    char *ret_buf = NULL;
    cJSON *item = cJSON_GetObjectItem(parent, name);
    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        ret_buf = (char *)malloc(strlen(item->valuestring) + 1);
        if (ret_buf != NULL) strcpy(ret_buf, item->valuestring);
    }
    return ret_buf;
}

/**
 * First try to find item with name `index`. Then try with name `def` if not found.
 * return -1 if nothing found.
 */
static double parse_index_based_number_config(cJSON* parent,
        const char *name, int index)
{
    int ret = -1;
    cJSON *item = cJSON_GetObjectItem(parent, name);
    if (!cJSON_IsObject(item)) {
        ALOGE("can not find object config item: %s", name);
    } else {
        char buf[50] = {0};
        sprintf(buf, "%d", index);
        cJSON *subitem = cJSON_GetObjectItem(item, buf);
        if (cJSON_IsNumber(subitem)) {
            ret = subitem->valuedouble;
        } else {
            subitem = cJSON_GetObjectItem(item, "def");
            if (cJSON_IsNumber(subitem)) {
                ret = subitem->valuedouble;
            }
        }
    }
    return ret;
}

static double parse_index_based_string_config(cJSON* parent,
        const char *name, char index[])
{
    int ret = -1;
    cJSON *item = cJSON_GetObjectItem(parent, name);
    if (!cJSON_IsObject(item)) {
        ALOGE("can not find object config item: %s", name);
    } else {
        cJSON *subitem = cJSON_GetObjectItem(item, index);
        if (cJSON_IsNumber(subitem)) {
            ret = subitem->valuedouble;
        } else {
            subitem = cJSON_GetObjectItem(item, "def");
            if (cJSON_IsNumber(subitem)) {
                ret = subitem->valuedouble;
            }
        }
    }
    return ret;
}

static double parse_mem_based_number_config(cJSON* parent,
        const char *name, int mem /* GB */)
{
    return parse_index_based_number_config(parent, name, mem);
}

static double parse_mem_based_number_config(cJSON* parent,
        const char *name, int mem /* GB */, int data /* GB */)
{
    char index[50] = {0};
    if(mem <= 6 && data <= 128) {
        sprintf(index, "%d+%d", mem, data);
    } else {
        sprintf(index, "high_device");
    }
    return parse_index_based_string_config(parent, name, index);
}

static double parse_cpu_based_number_config(cJSON* parent,
        const char *name, int cpus /* number */)
{
    return parse_index_based_number_config(parent, name, cpus);
}

static void parse_system_config(cJSON *parent, struct sys_info *si)
{
    double num_val = -1;
    char* str_val = NULL;
    if ((num_val = parse_number_config(parent, "swap_on")) >= 0) {
        sys_conf.zram.swap_on = (int)num_val;
        ALOGI("       swap_on: %d", (int)num_val);
    }
    if ((str_val = parse_string_config(parent, "comp_algo")) != NULL) {
        sys_conf.zram.comp_algorithm = str_val;
        ALOGI("       comp_algo: %s", str_val);
    }
    if ((num_val = parse_number_config(parent, "global_swappiness")) >= 0) {
        sys_conf.zram.swappiness = (int)num_val;
        ALOGI("       global_swappiness: %d", (int)num_val);
    }
    if ((num_val = parse_number_config(parent, "page_cluster")) >= 0) {
        sys_conf.zram.page_cluster = (int)num_val;
        ALOGI("       page_cluster: %d", (int)num_val);
    }
    if ((num_val = parse_number_config(parent, "max_comp_streams")) >= 0) {
        sys_conf.zram.max_comp_streams = (int)num_val;
        ALOGI("       max_comp_streams: %d", (int)num_val);
    }
    if ((num_val = parse_mem_based_number_config(parent, "zram_size", si->total_mem)) >= 0) {
        sys_conf.zram.disksize = (int)num_val;
        ALOGI("       zram_size: %d", (int)num_val);
    }

    if ((num_val = parse_number_config(parent, "extm_on")) >= 0) {
        sys_conf.zram.extm_on = (int)num_val;
        ALOGI("       extm_on: %d", (int)num_val);
    }
    if ((str_val = parse_string_config(parent, "extm_file")) != NULL) {
        sys_conf.zram.extm_file = str_val;
        ALOGI("       extm_file: %s", str_val);
    }
    if ((str_val = parse_string_config(parent, "extm_dev")) != NULL) {
        sys_conf.zram.extm_dev = str_val;
        ALOGI("       extm_dev: %s", str_val);
    }
    if ((num_val = parse_mem_based_number_config(parent, "extm_size", si->total_mem, si->total_data_size)) >= 0) {
        sys_conf.zram.extm_size = (int)num_val;
        ALOGI("       extm_size: %d", (int)num_val);
    }

    if ((num_val = parse_cpu_based_number_config(parent, "dex2oat_threads", si->nr_cpus)) >= 0) {
        sys_conf.dex2oat.threads = (int)num_val;
        ALOGI("       dex2oat_threads: %d", (int)num_val);
    }
    if ((num_val = parse_cpu_based_number_config(parent, "boot_dex2oat_threads", si->nr_cpus)) >= 0) {
        sys_conf.dex2oat.boot_threads = (int)num_val;
        ALOGI("       boot_dex2oat_threads: %d", (int)num_val);
    }
    if ((num_val = parse_cpu_based_number_config(parent, "bg_dex2oat_threads", si->nr_bg_cpus)) >= 0) {
        sys_conf.dex2oat.bg_threads = (int)num_val;
        ALOGI("       bg_dex2oat_threads: %d", (int)num_val);
    }

    if ((num_val = parse_mem_based_number_config(parent, "reclaim_on_start", si->total_mem)) >= 0) {
        sys_conf.mem.reclaim_on_start = (int)num_val;
        ALOGI("       reclaim_on_start: %d", (int)num_val);
    }
    if ((num_val = parse_mem_based_number_config(parent, "swappiness_on_start", si->total_mem)) >= 0) {
        sys_conf.mem.swappiness_on_start = (int)num_val;
        ALOGI("       swappiness_on_start: %d", (int)num_val);
    }
    if ((num_val = parse_mem_based_number_config(parent, "reclaim_on_launcher", si->total_mem)) >= 0) {
        sys_conf.mem.reclaim_on_launcher = (int)num_val;
        ALOGI("       reclaim_on_launcher: %d", (int)num_val);
    }
    if ((num_val = parse_mem_based_number_config(parent, "swappiness_on_launcher", si->total_mem)) >= 0) {
        sys_conf.mem.swappiness_on_launcher = (int)num_val;
        ALOGI("       swappiness_on_launcher: %d", (int)num_val);
    }
}

static void parse_specific_config(cJSON *parent, struct sys_info *si)
{
    cJSON* device_item = NULL;
    cJSON* model_item = NULL;
    cJSON* item = NULL;
    bool match = false;
    cJSON_ArrayForEach(device_item, parent)
    {
        model_item = cJSON_GetObjectItem(device_item, "product_name");
        cJSON_ArrayForEach(item, model_item)
        {
            if (cJSON_IsString(item) &&
                    strcmp(item->valuestring, si->product_name) == 0) {
                ALOGD("found product config:\n %s", cJSON_Print(device_item));
                match = true;
                break;
            }
        }
        if (match) {
            parse_system_config(device_item, si);
            break;
        }
    }
}

static void reset_sys_config()
{
    sys_conf.dex2oat.bg_threads = -1;
    sys_conf.dex2oat.boot_threads = -1;
    sys_conf.dex2oat.threads = -1;

    sys_conf.mem.reclaim_on_launcher = -1;
    sys_conf.mem.swappiness_on_launcher = -1;
    sys_conf.mem.reclaim_on_start = -1;
    sys_conf.mem.swappiness_on_start = -1;

    sys_conf.zram.swap_on = -1;
    sys_conf.zram.disksize = -1;
    sys_conf.zram.comp_algorithm = NULL;
    sys_conf.zram.swappiness = -1;
    sys_conf.zram.max_comp_streams = -1;
    sys_conf.zram.page_cluster = -1;

    sys_conf.zram.extm_on = -1;
    sys_conf.zram.extm_file = NULL;
    sys_conf.zram.extm_dev = NULL;
    sys_conf.zram.extm_size = -1;
}

void load_config(const char* config_path, struct sys_info *si)
{
    reset_sys_config();
    if (config_path == NULL) return;
    int fd = open(config_path, O_RDONLY);
    if (fd == -1) {
        ALOGE("open config file %s failed: %s.", config_path, strerror(errno));
        return;
    }

    struct stat stat_buf;
    if (fstat(fd, &stat_buf) == -1) {
        ALOGE("fstat config file %s failed: %s.", config_path, strerror(errno));
        return;
    }

    int file_size = stat_buf.st_size;
    char *str = (char *)mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
    if (str == (char *)-1) {
        ALOGE("mmap config file %s failed: %s.", config_path, strerror(errno));
        return;
    }

    cJSON *root = cJSON_Parse(str);
    if (root == NULL) {
        ALOGE("parse config file %s failed: %s.", config_path, strerror(errno));
        return;
    }

    cJSON *item = NULL;
    item = cJSON_GetObjectItem(root, "common");
    parse_system_config(item, si);

    std::string module[] = {"zram", "dex2oat"};

    for(int i = 0; i < sizeof(module) / sizeof(module[0]); i++) {
        item = cJSON_GetObjectItem(root, module[i].c_str());
        parse_specific_config(item, si);
    }

    available = CONFIG_AVAILABLE;
}

int loaded_config_available()
{
    return available;
}

struct mem_config *get_loaded_mem_config()
{
    return &(sys_conf.mem);
}

struct dex2oat_config *get_loaded_dex2oat_config()
{
    return &(sys_conf.dex2oat);
}

struct zram_config *get_loaded_zram_config()
{
    return &(sys_conf.zram);
}
