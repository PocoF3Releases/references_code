#define LOG_TAG "cirrus_spkcal"
//#define LOG_NDEBUG 0
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <media/AudioSystem.h>
#include <spkcal_config.h>

using namespace android;

static char fmtout_buf[512];
static void show_help(void)
{
    fprintf(stdout,
            "Usage: spkcal [-h][-c][-m][-d]\n\n"
            "App to do Cirrus Ampilfier Speaker Calibration\n\n"
            "Where:\n"
            "-c     Run calibration\n"
            "-m     Read value from file system\n"
            "-d     Clean value in file system\n\n"
            "-r     Set new value and apply\n\n"
            "-h     Shows this Help\n"
    );
}

static struct option longopts[] = {
    {"calib-set",       required_argument,  0,  'r'},
    {"calib-read",      no_argument,        0,  'm'},
    {"calib-clean",     no_argument,        0,  'd'},
    {"calib-run",       no_argument,        0,  'c'},
    {0,                 0,                  0,  0}
};

static int params_parser_int(const char* params, const char* target, int *out)
{
    char *buff = strdup(params);
    char *token = strtok(buff, ";");
    char *t;

    while (token != NULL)
    {
        t = strstr(token, target);
        if(t) {
            /*remove 'state=' in string 'state=xxxxxxxxxx'*/
            t = t + strlen(target) + 1;
            *out = strtol(t, NULL, 10);
            break;
        }
        token = strtok(NULL, ";");
    }

    free(buff);
    return 0;
}

static int params_parser_str(const char* params, const char* target, void *outbuf, size_t size)
{
    char *buff = strdup(params);
    char *token = strtok(buff, ";");
    char *t;

    while (token != NULL)
    {
        t = strstr(token, target);
        if(t) {
            /*remove 'state=' in string 'state=xxxxxxxxxx'*/
            t = t + strlen(target) + 1;
            memcpy(outbuf, t, strlen(t) > size ? size : strlen(t));
            break;
        }
        token = strtok(NULL, ";");
    }

    free(buff);
    return 0;
}

static void wait_for_calib_done(void *outbuf, size_t size)
{
    String8 params;
    const char *str = NULL;
    int state = 0;

    while(true) {
        usleep(500 * 1000);
        params = AudioSystem::getParameters(String8("cirrus_speaker_info=get"));
        str = params.string();
        params_parser_int(str, "state", &state);
        if(state == CALIB_DONE
                || state == CALIB_FAILED
                || state == CALIB_IDLE) {
            params_parser_str(str, "calib", outbuf, size);
            break;
        }
    }
}

int create_file(const char * fname) {
    FILE *fptr = NULL;

    if (access(fname, F_OK) == 0) {
        return EXIT_SUCCESS;
    }

    fptr = fopen(fname, "w");
    if (NULL == fptr) {
        ALOGE("can't create %s, errno = %d, reason = %s", fname, errno, strerror(errno));
        return EXIT_FAILURE;
    }
    fclose(fptr);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    struct timeval tval_before, tval_after, tval_result;
    int optindex = 0;
    int c = 0;
    String8 params;

    gettimeofday(&tval_before, NULL);
    opterr = 0;

#ifdef FACTORY_BUILD
    {
        int ret_temp = 0;
        ret_temp = create_file(CALIB_FILE_BIN);
        if (EXIT_FAILURE == ret_temp) return EXIT_FAILURE;
        ret_temp = create_file(CALIB_FILE_TXT);
        if (EXIT_FAILURE == ret_temp) return EXIT_FAILURE;
        ret_temp = create_file(AW_FILE_BIN);
        if (EXIT_FAILURE == ret_temp) return EXIT_FAILURE;
        ret_temp = create_file(AW_FILE_TXT);
        if (EXIT_FAILURE == ret_temp) return EXIT_FAILURE;
    }
#endif

    while ((c = getopt_long(argc, argv, "hmcdr:", longopts, &optindex)) != -1) {
        switch (c) {
            case 'c':
                AudioSystem::getParameters(String8("cirrus_speaker_calib=run"));
                usleep(2000 * 1000);
                wait_for_calib_done(fmtout_buf, 512);
                fprintf(stdout, "\n%s\n", fmtout_buf);
                gettimeofday(&tval_after, NULL);
                timersub(&tval_after, &tval_before, &tval_result);
                fprintf(stdout, "calibation took: %ld.%06ld seconds\n",
                        (long int)tval_result.tv_sec,
                        (long int)tval_result.tv_usec);
                break;
            case 'm':
                wait_for_calib_done(fmtout_buf, sizeof(fmtout_buf));
                fprintf(stdout, "\n%s\n", fmtout_buf);
                break;
            case 'd':
            case 'r':
            case 'h':
                show_help();
                break;
            default:
                break;
        }
    }

    return EXIT_SUCCESS;
}
