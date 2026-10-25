
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <cutils/log.h>

#include "display_utils.h"


using std::to_string;
using std::string;
using std::fstream;

#define READ_BACK_PARAM_NUM    256
#define MIPI_REG_PATH "/sys/class/mi_display/disp-DSI-0/mipi_rw"
#define PRIMARY_BACKLIGHT_PATH "/sys/class/backlight/panel0-backlight/brightness"
#define SECONDARY_BACKLIGHT_PATH "/sys/class/backlight/panel1-backlight/brightness"
#define DOZE_BRIGHTNESS_PATH "/sys/class/drm/card0-DSI-1/doze_brightness"

#define LOG_TAG "DisplayFeatureHal"
#define LOG_NDEBUG 0

namespace android {

bool disp_log_en = false;
NOTIFY_CHANGED DisplayUtil::mCallback = NULL;

bool DisplayUtil::mDCCBEnable = false;
int DisplayUtil::matrix_inverse(float matrix[MATRIX_ORDER][2*MATRIX_ORDER])
{
    int i, j, k, l, ret = 0;
    float common_factor = 0.0f;

    /*  matrix is  a order * 2*order matrix
       *  matrix = [A | E] = p1*p2*p3*{E | A^}
    */
    if (matrix == NULL) {
        ALOGE("%s, matrix pointer is null!", __FUNCTION__);
        return -1;
    }

    /* first transfer to Upper triangular matrix */
    for (i = 0; i < ARRAY_SIZE - 1; i++) {
        for (j = i + 1; j < ARRAY_SIZE; j++) {
            if (fabs(matrix[j][i]) < FLT_EPSILON) {
                ALOGW("%s, matrix element is zero!", __FUNCTION__);
                continue;
            }

            common_factor = matrix[j][i];
            for (k = 0; k< 2 * ARRAY_SIZE; k++) {
                matrix[j][k] = matrix[j][k] * matrix[i][i] - common_factor*matrix[i][k];
            }
        }
    }

    /* diagonal normalization */
    for (i = 0; i < ARRAY_SIZE; i++) {
        if (fabs(matrix[i][i]) < FLT_EPSILON) {
            ALOGE("%s, matrix diagonal element is zero!", __FUNCTION__);
            return -1;
        }

        common_factor = matrix[i][i];
        for (j = 0; j< 2*ARRAY_SIZE; j++) {
            matrix[i][j] = matrix[i][j]/common_factor;
        }
    }

    /* transfer to unit matrix */
    for (i = ARRAY_SIZE - 1; i > 0; i--) {
        for (j = i - 1; j >= 0; j--) {
            if (fabs(matrix[j][i]) < FLT_EPSILON) {
                ALOGW("%s, matrix element is zero!", __FUNCTION__);
                continue;
            }

            common_factor = matrix[j][i];
            for (k = 0; k< 2 * ARRAY_SIZE; k++) {
                matrix[j][k] = matrix[j][k] - common_factor*matrix[i][k] ;
            }
        }
    }

    return ret;
}

int DisplayUtil::parseLine(const char *input, const char *delim, char *tokens[],
                        const unsigned int max_token, unsigned int *count)
{
    char *tmp_token = NULL;
    char *temp_ptr;
    unsigned int index = 0;
    if (!input) {
        return -1;
    }
    tmp_token = strtok_r(const_cast<char *>(input), delim, &temp_ptr);
    while (tmp_token && index < max_token) {
        tokens[index++] = tmp_token;
        tmp_token = strtok_r(NULL, delim, &temp_ptr);
    }
    *count = index;

    return 0;
}

int DisplayUtil::getPanelName(int device_node, char *panel_name)
{
    return 0;
}

int DisplayUtil::getPanelInfo(int device_node, struct PanelInfo *panel_info)
{
    return 0;
}

void DisplayUtil::getDisplayMode(int device_node, struct PanelInfo *panel_info)
{
    return;
}
int DisplayUtil::readPanelMaxLuminance(int &max_luminance)
{
    int fd = -1;
    int ret = -1;
    char buf[READ_BACK_PARAM_NUM] = {0};

    fd = open(DISP_PARAM_PATH, O_RDWR);
    if (fd < 0) {
        ALOGE("%s, open fb disp_param node failed!", __FUNCTION__);
        return ret;
    }

    ret = write(fd, (void*)DISP_READ_MAX_LUMINANCE, sizeof(DISP_READ_MAX_LUMINANCE));
    if (ret < 0) {
        ALOGE("%s, write disp_param node failed!", __FUNCTION__);
        goto FAILED;
    }

    /* set current pointer to begin */
    lseek(fd, 0, SEEK_SET);

    /* buf - buffer size must >= 256byte */
    ret = read(fd, (void *)buf, READ_BACK_PARAM_NUM);
    if (ret < 0) {
        ALOGE("%s, read disp_param node failed!", __FUNCTION__);
        goto FAILED;
    }

    /* parse parameters */
    sscanf(buf, "p0=%d", &max_luminance);
    ALOGV("buf:%s, max luminance:%d", buf, max_luminance);

FAILED:
    close(fd);

    return ret;

}

int DisplayUtil::readWPMaxBrightness(long *max_wp_brightness)
{
    int fd = -1;
    char buf[READ_BACK_PARAM_NUM] = {0};

    if (max_wp_brightness == NULL) {
        ALOGE("%s, invalied parameter!", __FUNCTION__);
        return -1;
    }

    fd = open(MAX_WP_BACKLIGHT_PATH, O_RDONLY);
    if (fd < 0) {
        ALOGE("%s, open fb wp_info node failed!", __FUNCTION__);
        return -1;
    }

   // fd = lseek(fd, 4, SEEK_SET);

    memset(buf, 0x0, sizeof(buf));
    if (read(fd, (void *)buf, READ_BACK_PARAM_NUM) <= 0){
        ALOGE("%s, read wp_info node failed!", __FUNCTION__);
        close(fd);
        return -1;
    } else {
        ALOGI("%s, read wp_info node success!", __FUNCTION__);
    }

    /* parse parameters */
    *max_wp_brightness = strtol(buf, NULL, 16) & 0xFF;
    ALOGI("%s, read max_wp_brightness:%ld", __FUNCTION__, *max_wp_brightness);

    close(fd);
    return 0;
}

int DisplayUtil::readMaxBrightness(long *max_brightness)
{
    int fd = -1;
    char buf[READ_BACK_PARAM_NUM] = {0};

    if (max_brightness == NULL) {
        ALOGE("%s, invalied parameter!", __FUNCTION__);
        return -1;
    }

    fd = open(MAX_BACKLIGHT_PATH, O_RDONLY);
    if (fd < 0) {
        ALOGE("%s, open fb max_brightness node failed!", __FUNCTION__);
        return -1;
    }

    memset(buf, 0x0, sizeof(buf));
    if (read(fd, (void *)buf, READ_BACK_PARAM_NUM) <= 0){
        ALOGE("%s, read max_brightness node failed!", __FUNCTION__);
        close(fd);
        return -1;
    }

    /* parse parameters */
    *max_brightness = strtol(buf, NULL, 10);
    ALOGI("read max_brightness:%d", *max_brightness);

    close(fd);
    return 0;
}


int DisplayUtil::readPanelWhiteXYCoordinate(int &x_coordinate, int &y_coordinate)
{
    int fd = -1;
    int ret = -1;
    char buf[READ_BACK_PARAM_NUM] = {0};

    fd = open(DISP_PARAM_PATH, O_RDWR);
    if (fd < 0) {
        ALOGE("%s, open fb disp_param node failed!", __FUNCTION__);
        return ret;
    }

    ret = write(fd, (void*)DISP_READ_WHITE_POINT, sizeof(DISP_READ_WHITE_POINT));
    if (ret < 0) {
        ALOGE("%s, write disp_param node failed!", __FUNCTION__);
        goto FAILED;
    }

    /* set current pointer to begin */
    lseek(fd, 0, SEEK_SET);

    /* buf - buffer size must >= 256byte */
    ret = read(fd, (void *)buf, READ_BACK_PARAM_NUM);
    if (ret < 0) {
        ALOGE("%s, read disp_param node failed!", __FUNCTION__);
        goto FAILED;
    }

    /* parse p3 and p4 parameters */
    sscanf(buf, "p0=%dp1=%d", &x_coordinate, &y_coordinate);
    ALOGV("buf:%s, x coordinate:%d, y coordinate:%d", buf, x_coordinate, y_coordinate);

FAILED:
    close(fd);

    return ret;
}

int DisplayUtil::readPanelNTC(int *temperature)
{
    int fd = -1;
    char buf[READ_BACK_PARAM_NUM] = {0};
    int i;
    char *temp_dir = NULL;
    char name[20] = "backlight_therm\n";
    int ret = 0;
    if (temperature == NULL) {
        ALOGE("%s, invalied parameter!", __FUNCTION__);
        return -1;
    }
    temp_dir = (char*)malloc(256*sizeof(char));
    if (temp_dir == NULL) {
        ALOGE("%s, Failed malloc", __FUNCTION__);
        return -1;
    }
    memset(temp_dir, 0, sizeof(256*sizeof(char)));
    for (i = 0; i < 100; i++) {
        sprintf(temp_dir, "sys/class/thermal/thermal_zone%d/type", i);
        fd = open(temp_dir, O_RDONLY);
        if (fd < 0) {
            ALOGE("%s, open panel thermal zone type node failed! %s", __FUNCTION__, temp_dir);
            ret = -1;
            goto EXIT;
        }
        memset(buf, 0x0, sizeof(buf));
        if (read(fd, (void *)buf, READ_BACK_PARAM_NUM) <= 0){
            ALOGE("%s, read panel temperature node failed!", __FUNCTION__);
            ret = -1;
            goto EXIT1;
        }
        if (!strncmp(name, buf, 15)) {
            close(fd);
            fd = -1;
            sprintf(temp_dir, "sys/class/thermal/thermal_zone%d/temp", i);
            fd = open(temp_dir, O_RDONLY);
            if (fd < 0) {
                ALOGE("%s, open panel thermal zone temp node failed!", __FUNCTION__, temp_dir);
                ret = -1;
                goto EXIT;
            }
            memset(buf, 0x0, sizeof(buf));
            if (read(fd, (void *)buf, READ_BACK_PARAM_NUM) <= 0){
                ALOGE("%s, read panel temperature node failed!", __FUNCTION__);
                ret = -1;
                goto EXIT1;
            }

            /* parse parameters */
            *temperature = strtol(buf, NULL, 10);
            ALOGI("read panel temperature:%d", *temperature);
            ret = 0;
            goto EXIT1;
        } else {
            close(fd);
            fd = -1;
        }
    }
EXIT1:
    if (fd >= 0)
        close(fd);
EXIT:
    free(temp_dir);
    temp_dir = NULL;
    return ret;
}

int DisplayUtil::readPanelTemp(int *temperature)
{
    int fd = -1, ret = -1;
    char buf[READ_BACK_PARAM_NUM] = {0};

    if (temperature == NULL) {
        ALOGE("%s, invalied parameter!", __FUNCTION__);
        return ret;
    }

    fd = open(DISP_NTC_PATH, O_RDONLY);
    if (fd < 0) {
        ALOGE("%s, open panel thermal zone type node failed! %s", __FUNCTION__, DISP_NTC_PATH);
        return ret;
    }

    memset(buf, 0x0, sizeof(buf));
    if (read(fd, (void *)buf, READ_BACK_PARAM_NUM) <= 0){
        ALOGE("%s, read panel temperature node failed!", __FUNCTION__);
    } else {
        /* parse parameters */
        *temperature = strtol(buf, NULL, 10) / 1000;
        ALOGI("read panel temperature:%d", *temperature);
        ret = 0;
    }

    close(fd);
    return ret;
}

int DisplayUtil::writePanelDsiCmds(char* cmds)
{
    FILE* fp;

    if (cmds == NULL) {
        ALOGE("invalid param");
        return -1;
    }

    fp = fopen(MIPI_REG_PATH, "w");
    if (fp == NULL) {
        ALOGE("open file error: %s", MIPI_REG_PATH);
        return -1;
    }

    if (fwrite(cmds, strlen(cmds) + 1, 1, fp) < 0) {
        ALOGE("write file error!");
        fclose(fp);
        return -1;
    }

    ALOGV("successfully write %s into %s", cmds, MIPI_REG_PATH);

    fclose(fp);
    return 0;

}

int DisplayUtil::writeDCBacklight(char* value)
{
    int fd = -1;
    int ret = -1;

    ALOGV("%s setting backlight to %s", __func__, value);
    fd = open(PRIMARY_BACKLIGHT_PATH, O_RDWR);
    if (fd < 0) {
        ALOGE("open backlight node error: %s", PRIMARY_BACKLIGHT_PATH);
        return -1;
    }

    ret = write(fd, (void*)value, strlen(value));
    if (ret < 0) {
        ALOGE("%s, write (%s) backlight node failed ret=%d!", __FUNCTION__, value, ret);
        goto FAILED;
    }

    ALOGV("successfully write %s into %s", value, PRIMARY_BACKLIGHT_PATH);

FAILED:
    close(fd);
    return ret;
}

int DisplayUtil::writeDualBacklight(char* value)
{
    int fd = -1;
    int ret = -1;

    ALOGV("%s setting backlight to %s", __func__, value);
    fd = open(PRIMARY_BACKLIGHT_PATH, O_RDWR);
    if (fd < 0) {
        ALOGE("open backlight node error: %s", PRIMARY_BACKLIGHT_PATH);
        goto SECOND;
    }

    ret = write(fd, (void*)value, strlen(value));
    if (ret < 0) {
        ALOGE("%s, writeP (%s) backlight node failed ret=%d!", __FUNCTION__, value, ret);
        goto FAILED;
    }

    ALOGV("successfully write %s into %s", value, PRIMARY_BACKLIGHT_PATH);

FAILED:
    close(fd);
    fd = -1;

SECOND:
    if (access(SECONDARY_BACKLIGHT_PATH, F_OK) < 0) {
        return 0;
    }
    fd = open(SECONDARY_BACKLIGHT_PATH, O_RDWR);
    if (fd < 0) {
        ALOGE("open backlight node error: %s", SECONDARY_BACKLIGHT_PATH);
        return -1;
    }

    ret = write(fd, (void*)value, strlen(value));
    if (ret < 0) {
        ALOGE("%s, writeS (%s) backlight node failed ret=%d!", __FUNCTION__, value, ret);
        goto FAILED1;
    }

    ALOGV("successfully write %s into %s", value, SECONDARY_BACKLIGHT_PATH);
FAILED1:
    close(fd);
    return ret;
}

int DisplayUtil::writeDozeBrightness(char* value)
{
    int fd = -1;
    int ret = -1;

    ALOGD("%s doze brightness to %s", __func__, value);
    fd = open(DOZE_BRIGHTNESS_PATH, O_RDWR);
    if (fd < 0) {
        ALOGE("open doze_brightness node error: %s", DOZE_BRIGHTNESS_PATH);
        return -1;
    }

    ret = write(fd, (void*)value, strlen(value));
    if (ret < 0) {
        ALOGE("%s, write (%s) backlight node failed ret=%d!", __FUNCTION__, value, ret);
        goto FAILED;
    }

    ALOGD("successfully write %s into %s", value, DOZE_BRIGHTNESS_PATH);

FAILED:
    close(fd);

    return ret;
}


void DisplayUtil::cct_to_xy(const double cct, double *x, double *y)
{
    if(cct>7000) {
        *x = -2.0064*(1.0e9/pow(cct,3))+1.9018*(1.0e6/pow(cct,2))+0.24748*(1.0e3/cct)+0.23704;
    } else {
        *x = -4.607*(1.0e9/pow(cct,3))+2.9678*(1.0e6/pow(cct,2))+0.09911*(1.0e3/cct)+0.244063;
    }
    *y = -3*pow(*x,2)+2.87*(*x)-0.275;
}

void DisplayUtil::xy_to_XYZ(double x, double y,
    double *xXYZ, double *yXYZ, double *zXYZ)
{
    *yXYZ = 1;
    *xXYZ =  x * (*yXYZ)/y;
    *zXYZ = (1-x-y)*(*yXYZ)/y;
    //ALOGD("X:%.16f Y:%.16f Z:%.16f\n",*xXYZ,*xXYZ,*xXYZ);
}

void DisplayUtil::xy_to_cct(double x, double y, double *cct)
{
    *cct = -437*pow(((x-0.332)/(y-0.1858)),3)+3601*pow((x-0.332)/(y-0.1858),2)-6831*((x-0.332)/(y-0.1858))+5517;
    //*cct = 449 * pow((x - 0.3320)/(0.1858 - y), 3) + 3525 * pow((x - 0.3320)/(0.1858 - y), 2) + 6823.3 * ((x - 0.3320)/(0.1858 - y)) +5520.33;
    //ALOGD("x:%.16f y:%.16f test_cct:%.16f\n",x,y,*cct);
}

void DisplayUtil::xy_to_ulvl(double x, double y, double *ul, double *vl)
{
    *ul = 4*x/(-2*x+12*y+3);
    *vl = 9*y/(-2*x+12*y+3);
    //ALOGD("\nul:%.16f vl:%.16f\n",*ul,*vl);
}

void DisplayUtil::ulvl_to_xy(double ul,double vl,double *xl,double *yl)
{
    *xl = (4.5*ul)/(3*ul-8*vl+6);
    *yl = (2*vl)/(3*ul-8*vl+6);
    //ALOGD("\nxl:%.16f yl:%.16f\n",*xl,*yl);
}

void DisplayUtil::registerCallback(void *callback)
{
    if (callback) {
        mCallback = (NOTIFY_CHANGED)callback;
    }
}

bool DisplayUtil::isRegisterCallbackDone()
{
    return (mCallback ? true : false);
}

void DisplayUtil::onCallback(int display_id, int value, float red, float green, float blue)
{
    if (display_id == 40000 && !mDCCBEnable)
        return;
    if (mCallback) {
        mCallback(display_id, value, red, green, blue);
    }
}

void DisplayUtil::setDCParseCallbackEnable(bool enable)
{
    ALOGD("Setting mDCCBEnable %d", enable);
    mDCCBEnable = enable;
}

}
