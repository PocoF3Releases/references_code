#ifndef MC_PLUGINTOOLS_H
#define MC_PLUGINTOOLS_H

#define OUTPUT_PATH_OVERLAY     "/sdcard/klo/"
#define PLUGIN_TOOL_PATH        "/system/lib/"

typedef void * (*plugintool_func)(void *);
typedef void (*plugintool_dump_func)(int);
struct plugin_entity {
    int  magic;
    char *description;
    //will start thread to do the plugin job, param is the member void *param
    plugintool_func start_func;
    //optional param is the member void *param,return minus(<0) code means stop meet error
    plugintool_func stop_func;
    //optional, param is fd pointer, none check return code
    plugintool_dump_func dump_func;
    void *param;
    char *input_path;
    char *output_path;
    int  default_start;
}__attribute__((packed));

struct mc_plugintool {
    struct plugin_entity  tool_entity;
    unsigned int   tool_id;
    unsigned int   tool_started;    //valid for un-self quit plugin thread
    int            tool_tid;
    char          *tool_path;
    struct mc_plugintool * next;
}__attribute__((packed));

int mc_plugintool_register(struct plugin_entity *my_plugintool);
int mc_plugintool_unregister(unsigned int tool_id);
int mc_plugintool_mod_param(unsigned int tool_id, void *param);
int mc_plugintool_output_overlay(unsigned int tool_id, char *path);
int mc_plugintool_stop(unsigned int tool_id);
int mc_plugintool_start(unsigned int tool_id);
int mc_plugintool_dump_info(int fd, unsigned int tool_id);
int mc_plugintool_load(char *full_path);
int mc_plugintool_load_all();
struct mc_plugintool * mc_plugintool_get_plugin_list();

#endif
