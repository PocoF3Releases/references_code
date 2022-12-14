/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <string.h>
#include <unistd.h>
#include <string>

using namespace std;

int show_ip(char *fname, string dstport);
int strstart(char **a, char *b);
int cnss_statistic_genl_init(void);
void cnss_statistic_genl_exit(void);
int cnss_statistic_genl_recvmsgs(void);
int cnss_statistic_genl_get_fd(void);
int cnss_statistic_send_debug_command(void);
int cnss_statistic_send_enable_command(void);
int cnss_statistic_send_disable_command(void);
