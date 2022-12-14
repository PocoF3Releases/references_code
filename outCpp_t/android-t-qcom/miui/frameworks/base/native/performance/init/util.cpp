
#include "init.h"

void set_int_prop_config_item(const char *prop, int config)
{
    char prop_buf[PROPERTY_VALUE_MAX];

    property_get(prop, prop_buf, "");
    if (prop_buf[0] != '\0') {
        ALOGI("%s already set as %s", prop, prop_buf);
        return;
    }
    sprintf(prop_buf, "%d", config);
    property_set(prop, prop_buf);
    return;
}

void set_int_prop_config_item_forcing(const char *prop, int config)
{
    char prop_buf[PROPERTY_VALUE_MAX];

    ALOGI("%s will set as %d", prop, config);
    sprintf(prop_buf, "%d", config);
    property_set(prop, prop_buf);
    return;
}

int get_int_prop_config_item(const char *prop, int default_val)
{
    char prop_buf[PROPERTY_VALUE_MAX];

    property_get(prop, prop_buf, "");
    if (prop_buf[0] != '\0') {
        ALOGI("%s already set as %s", prop, prop_buf);
        return atoi(prop_buf);
    }
    ALOGI("%s has been set as default %d", prop, default_val);
    return default_val;
}

long long get_long_long_prop_config_item(const char *prop, long long default_val)
{
    char prop_buf[PROPERTY_VALUE_MAX];

    property_get(prop, prop_buf, "");
    if (prop_buf[0] != '\0') {
        ALOGI("%s already set as %s", prop, prop_buf);
        return atoll(prop_buf);
    }
    ALOGI("%s has been set as default %lld", prop, default_val);
    return default_val;
}

int write_fmt_data_to_file(const char *filename, const char* fmt, ...)
{
    int ret = 0;
    int fd = TEMP_FAILURE_RETRY(open(filename, O_WRONLY));
    if (fd == -1) {
        ALOGE("Unable to open %s: %s", filename, strerror(errno));
        ret = -1;
        goto end;
    }
    va_list ap;
    va_start(ap, fmt);
    if (vdprintf(fd, fmt, ap) < 0) {
        ALOGE("Failed write to %s: %s", filename, strerror(errno));
        ret = -2;
    }
    va_end(ap);
end:
    close(fd);
    return ret;
}

int wait_for_file(const char* filename, std::chrono::milliseconds timeout) {
    android::base::Timer t;
    while (t.duration() < timeout) {
        struct stat sb;
        if (stat(filename, &sb) != -1) {
            ALOGI("wait for '%s' took %lldms.", filename, t.duration().count());
            return 0;
        }
        std::this_thread::sleep_for(10ms);
    }
    ALOGW("wait for '%s' timed out and took %lldms.", filename, t.duration().count());
    return -1;
}