#ifndef XLOG_MSG
#define XLOG_MSG
int format_msg_struct(char *dst, int msg_id, struct xlog_flat_msg *log);
int format_msg_str(char *dst, int msg_id, char *arg);
int format_msg_int(char *dst, int msg_id, int *xlog_value);
#endif