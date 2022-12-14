typedef unsigned int uint32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint64;


char * shellcmd(char *cmd, char *buff, int size);
void qmesa_err_pare(char **err_buf);
char *set_test_mem();
int *switch_freq_list();
void bimc_freq_switch(int test_time);
